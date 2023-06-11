//@	{"target":{"name":"http_writer_response_body.test"}}

#include "./http_write_response_body.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct sink
	{
		west::io::write_result write(std::span<char const> buffer)
		{
			auto const bytes_to_write = std::size(buffer);
			output.insert(std::end(output), std::begin(buffer), std::end(buffer));

			return west::io::write_result{
				bytes_to_write,
				west::io::operation_result::completed
			};
		}

		std::string output;
	};

	struct bad_sink
	{
		west::io::write_result write(std::span<char const>)
		{
			return west::io::write_result{
				0,
				west::io::operation_result::operation_would_block
			};
		}

		std::string output;
	};

	enum class error_code{no_error, would_block, error};

	struct read_result
	{
		size_t bytes_read;
		error_code ec;
	};

	constexpr bool can_continue(error_code ec)
	{
		switch(ec)
		{
			case error_code::no_error:
				return true;
			case error_code::would_block:
				return true;
			case error_code::error:
				return false;
			default:
				__builtin_unreachable();
		}
	}

	constexpr bool is_error_indicator(error_code ec)
	{
		switch(ec)
		{
			case error_code::no_error:
				return false;
			case error_code::would_block:
				return false;
			case error_code::error:
				return true;
			default:
				__builtin_unreachable();
		}
	}

	constexpr char const* to_string(error_code ec)
	{
		switch(ec)
		{
			case error_code::no_error:
				return "No error";
			case error_code::would_block:
				return "Would block";
			case error_code::error:
				return "Error";
			default:
				__builtin_unreachable();
		}
	}

	struct request_handler
	{
		explicit request_handler(std::string_view sv):
			read_pos{std::data(sv)},
			end_ptr{std::data(sv) + std::size(sv)}
		{}

		read_result read_response_content(std::span<char> buffer)
		{
			auto const n_bytes_left = static_cast<size_t>(end_ptr - read_pos);
			auto const bytes_to_write = std::min(std::size(buffer), n_bytes_left);
			std::copy_n(read_pos, bytes_to_write, std::begin(buffer));
			read_pos += bytes_to_write;

			return read_result{
				bytes_to_write,
				error_code::no_error
			};
		};

		char const* read_pos;
		char const* end_ptr;
	};

	struct blocking_request_handler
	{
		read_result read_response_content(std::span<char>)
		{
			return read_result{
				0,
				error_code::would_block
			};
		};
	};

	struct failing_request_handler
	{
		read_result read_response_content(std::span<char>)
		{
			return read_result{
				0,
				error_code::error
			};
		};
	};
}

TESTCASE(http_write_response_body_write_all_data)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_sapn{buffer};

	std::string_view src{
"Etiam egestas ex laoreet tortor tristique, vitae tristique enim vehicula. Aenean mollis tristique "
"eros nec malesuada. Suspendisse bibendum maximus erat, id volutpat enim. Phasellus at pharetra "
"elit, ut molestie velit. Phasellus sed lacinia risus. Pellentesque condimentum, arcu at ultricies "
"egestas, dui lectus fringilla dui, et pellentesque lorem enim at arcu. Pellentesque sed tortor a "
"tortor sollicitudin dapibus nec sed nulla. Nulla laoreet vel eros ut venenatis. Praesent aliquam "
"pharetra scelerisque. Fusce ut ex fermentum, laoreet ex ut, sollicitudin metus. Praesent luctus "
"purus at eleifend ullamcorper. Integer viverra ipsum enim, eu sollicitudin diam rhoncus in. "
};

	west::http::session session{sink{},
		request_handler{src},
		west::http::request_info{},
		west::http::response_header{}
	};

	west::http::write_response_body writer{std::size(src)};

	auto res = writer.socket_is_ready(buff_sapn, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.state_result.http_status, west::http::status::ok);
	EXPECT_EQ(res.state_result.error_message, nullptr);
	EXPECT_EQ(session.connection.output, src);
}

TESTCASE(http_write_response_body_write_failed)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_sapn{buffer};

	std::string_view src{
"Etiam egestas ex laoreet tortor tristique, vitae tristique enim vehicula. Aenean mollis tristique "
"eros nec malesuada. Suspendisse bibendum maximus erat, id volutpat enim. Phasellus at pharetra "
"elit, ut molestie velit. Phasellus sed lacinia risus. Pellentesque condimentum, arcu at ultricies "
"egestas, dui lectus fringilla dui, et pellentesque lorem enim at arcu. Pellentesque sed tortor a "
"tortor sollicitudin dapibus nec sed nulla. Nulla laoreet vel eros ut venenatis. Praesent aliquam "
"pharetra scelerisque. Fusce ut ex fermentum, laoreet ex ut, sollicitudin metus. Praesent luctus "
"purus at eleifend ullamcorper. Integer viverra ipsum enim, eu sollicitudin diam rhoncus in. "
};

	west::http::session session{bad_sink{},
		request_handler{src},
		west::http::request_info{},
		west::http::response_header{}
	};

	west::http::write_response_body writer{std::size(src)};

	auto res = writer.socket_is_ready(buff_sapn, session);

	EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
	EXPECT_EQ(res.state_result.http_status, west::http::status::ok);
	EXPECT_EQ(res.state_result.error_message, nullptr);
}

TESTCASE(http_write_response_body_blocking_request_handler)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_sapn{buffer};

	std::string_view src{
"Etiam egestas ex laoreet tortor tristique, vitae tristique enim vehicula. Aenean mollis tristique "
"eros nec malesuada. Suspendisse bibendum maximus erat, id volutpat enim. Phasellus at pharetra "
"elit, ut molestie velit. Phasellus sed lacinia risus. Pellentesque condimentum, arcu at ultricies "
"egestas, dui lectus fringilla dui, et pellentesque lorem enim at arcu. Pellentesque sed tortor a "
"tortor sollicitudin dapibus nec sed nulla. Nulla laoreet vel eros ut venenatis. Praesent aliquam "
"pharetra scelerisque. Fusce ut ex fermentum, laoreet ex ut, sollicitudin metus. Praesent luctus "
"purus at eleifend ullamcorper. Integer viverra ipsum enim, eu sollicitudin diam rhoncus in. "
};

	west::http::session session{sink{},
		blocking_request_handler{},
		west::http::request_info{},
		west::http::response_header{}
	};

	west::http::write_response_body writer{std::size(src)};

	auto res = writer.socket_is_ready(buff_sapn, session);

	EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
	EXPECT_EQ(res.state_result.http_status, west::http::status::ok);
	EXPECT_EQ(res.state_result.error_message, nullptr);
}

TESTCASE(http_write_response_body_failing_request_handler)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_sapn{buffer};

	std::string_view src{
"Etiam egestas ex laoreet tortor tristique, vitae tristique enim vehicula. Aenean mollis tristique "
"eros nec malesuada. Suspendisse bibendum maximus erat, id volutpat enim. Phasellus at pharetra "
"elit, ut molestie velit. Phasellus sed lacinia risus. Pellentesque condimentum, arcu at ultricies "
"egestas, dui lectus fringilla dui, et pellentesque lorem enim at arcu. Pellentesque sed tortor a "
"tortor sollicitudin dapibus nec sed nulla. Nulla laoreet vel eros ut venenatis. Praesent aliquam "
"pharetra scelerisque. Fusce ut ex fermentum, laoreet ex ut, sollicitudin metus. Praesent luctus "
"purus at eleifend ullamcorper. Integer viverra ipsum enim, eu sollicitudin diam rhoncus in. "
};

	west::http::session session{sink{},
		failing_request_handler{},
		west::http::request_info{},
		west::http::response_header{}
	};

	west::http::write_response_body writer{std::size(src)};

	auto res = writer.socket_is_ready(buff_sapn, session);

	EXPECT_EQ(res.status, west::http::session_state_status::write_response_failed);
	EXPECT_EQ(res.state_result.http_status, west::http::status::internal_server_error);
	EXPECT_EQ(res.state_result.error_message.get(), std::string_view{"Error"});
}