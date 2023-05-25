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
				west::io::operation_result::object_is_still_ready
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

	enum class error_code{no_error};

	struct read_result
	{
		size_t bytes_read;
		error_code ec;
	};

	constexpr bool can_continue(error_code)
	{
		return true;
	}

	constexpr bool is_error_indicator(error_code)
	{
		return false;
	}

	constexpr char const* to_string(error_code)
	{
		return "No error";
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
		west::http::request_header{},
		west::http::response_header{}
	};

	west::http::write_response_body writer{std::size(src)};

	auto res = writer(buff_sapn, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::ok);
	EXPECT_EQ(res.error_message, nullptr);
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
		west::http::request_header{},
		west::http::response_header{}
	};

	west::http::write_response_body writer{std::size(src)};

	auto res = writer(buff_sapn, session);

	EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
	EXPECT_EQ(res.http_status, west::http::status::ok);
	EXPECT_EQ(res.error_message, nullptr);
}