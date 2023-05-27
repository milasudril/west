//@	{"target":{"name":"http_request_processor.test"}}

#include "./http_request_processor.hpp"

#include <testfwk/testfwk.hpp>
#include <random>

namespace
{
	class socket
	{
	public:
		auto read(std::span<char> buffer)
		{
			if((m_endpoint_status & CLIENT_WRITE_CLOSED) || (m_endpoint_status & SERVER_READ_CLOSED))
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::completed
				};
			}
			else
			if(m_endpoint_status & READ_TRIGGERS_ERROR)
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::error
				};
			}
			else
			{
				if(m_calls_to_read % m_read_blocks_every != 0)
				{
					++m_calls_to_read;
					auto const bytes_left = static_cast<size_t>(std::end(m_request) - m_request_offset);
					auto const bytes_to_read = std::min(std::min(std::size(buffer), bytes_left), static_cast<size_t>(23));
					std::copy_n(m_request_offset, bytes_to_read, std::begin(buffer));
					m_request_offset += bytes_to_read;
					return west::io::read_result{
						.bytes_read = bytes_to_read,
						.ec = bytes_to_read != 0 ? west::io::operation_result::object_is_still_ready
							: west::io::operation_result::completed
					};
				}
				else
				{
					++m_calls_to_read;
					return west::io::read_result{
						.bytes_read = 0,
						.ec = west::io::operation_result::operation_would_block
					};
				}
			}
		}

		auto write(std::span<char const> buffer)
		{
			if(m_endpoint_status & WRITE_TRIGGERS_ERROR)
			{
				return west::io::write_result{
					.bytes_written = 0,
					.ec = west::io::operation_result::error
				};
			}
			else
			{
				m_output.insert(std::end(m_output), std::begin(buffer), std::end(buffer));
				return west::io::write_result{
					.bytes_written = std::size(buffer),
					.ec = west::io::operation_result::object_is_still_ready
				};
			}
		}

		void stop_reading()
		{ m_endpoint_status |= SERVER_READ_CLOSED; }

		bool server_read_closed() const { return m_endpoint_status & SERVER_READ_CLOSED; }

		void client_stop_read()
		{ m_endpoint_status |= CLIENT_READ_CLOSED; }

		void client_stop_write()
		{ m_endpoint_status |= CLIENT_WRITE_CLOSED; }

		void enable_read_error()
		{ m_endpoint_status |= READ_TRIGGERS_ERROR; }

		void enable_write_error()
		{ m_endpoint_status |= WRITE_TRIGGERS_ERROR; }

		auto& output() const
		{ return m_output; }

		void reset()
		{
			m_endpoint_status = 0;
			m_output.clear();
		}

		void request(std::string_view req)
		{
			m_request = req;
			m_request_offset = std::begin(req);
		}

		void read_blocks(size_t n)
		{
			assert(n >= 2);
			m_read_blocks_every = n;
		}

	private:
		uint32_t m_endpoint_status{0};
		static constexpr uint32_t CLIENT_READ_CLOSED{1};
		static constexpr uint32_t CLIENT_WRITE_CLOSED{2};
		static constexpr uint32_t SERVER_READ_CLOSED{4};
		static constexpr uint32_t SERVER_WRITE_CLOSED{8};
		static constexpr uint32_t READ_TRIGGERS_ERROR{16};
		static constexpr uint32_t WRITE_TRIGGERS_ERROR{32};

		std::string_view m_request;
		std::string_view::iterator m_request_offset;
		std::string m_output;

		size_t m_read_blocks_every{65536};
		size_t m_calls_to_read{0};
	};




	enum class request_handler_error_code{no_error, would_block, error};

	struct read_result
	{
		size_t bytes_read;
		request_handler_error_code ec;
	};

	constexpr bool can_continue(request_handler_error_code ec)
	{
		switch(ec)
		{
			case request_handler_error_code::no_error:
				return true;
			case request_handler_error_code::would_block:
				return true;
			case request_handler_error_code::error:
				return false;
			default:
				__builtin_unreachable();
		}
	}

	constexpr bool is_error_indicator(request_handler_error_code ec)
	{
		switch(ec)
		{
			case request_handler_error_code::no_error:
				return false;
			case request_handler_error_code::would_block:
				return false;
			case request_handler_error_code::error:
				return true;
			default:
				__builtin_unreachable();
		}
	}

	constexpr char const* to_string(request_handler_error_code ec)
	{
		switch(ec)
		{
			case request_handler_error_code::no_error:
				return "No error";
			case request_handler_error_code::would_block:
				return "Would block";
			case request_handler_error_code::error:
				return "Error";
			default:
				__builtin_unreachable();
		}
	}

	struct request_handler_write_result
	{
		size_t bytes_written;
		request_handler_error_code ec;
	};

	struct request_handler_read_result
	{
		size_t bytes_read;
		request_handler_error_code ec;
	};

	class request_handler
	{
	public:
		explicit request_handler(std::string_view response_body):
			m_response_body{response_body},
			m_read_offset{std::begin(m_response_body)}
		{}

		auto finalize_state(west::http::request_header const& header)
		{
			puts("Finalize read header");
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = nullptr;

			if(auto const i = header.fields.find("Trigger-Rej-By-App"); i != std::end(header.fields))
			{
				if(i->second == "At header validation")
				{
					validation_result.http_status = west::http::status::i_am_a_teapot;
					validation_result.error_message = west::make_unique_cstr("Application requested request to be reject when validating the header");
				}
				else
				{ m_rej_req = true; }
			}

			m_fail_writing_response = header.fields.contains("Fail-Writing-Response");

			return validation_result;
		}

		auto finalize_state(west::http::field_map& fields)
		{
			puts("Finalize read body");
			west::http::finalize_state_result validation_result;
			fields.append("Content-Length", std::to_string(std::size(m_response_body)));
			if(m_rej_req)
			{
				validation_result.http_status = west::http::status::unprocessable_content;
				validation_result.error_message = west::make_unique_cstr("Application requested request to be reject when the body was processed");
			}
			else
			{
				validation_result.http_status = west::http::status::ok;
				validation_result.error_message = nullptr;
			}
			return validation_result;
		}

		void finalize_state(west::http::field_map& fields, west::http::session_state_response&& res)
		{
			m_error_message = std::move(res.error_message);
			m_response_body = std::string_view{m_error_message.get()};
			m_read_offset = std::begin(m_response_body);
			fields.append("Content-Length", std::to_string(std::size(m_response_body)));
		}

		auto process_request_content(std::span<char const> buffer)
		{
			m_request.insert(std::end(m_request), std::begin(buffer), std::end(buffer));
			return request_handler_write_result{
				.bytes_written = std::size(buffer),
				.ec = request_handler_error_code::no_error
			};
		}

		auto read_response_content(std::span<char> buffer)
		{
			auto const n_bytes_left = static_cast<size_t>(std::end(m_response_body) - m_read_offset);
			auto const bytes_to_read = std::min(std::size(buffer), n_bytes_left);
			std::copy_n(m_read_offset, bytes_to_read, std::begin(buffer));
			m_read_offset += bytes_to_read;

			return request_handler_read_result{
				bytes_to_read,
				m_fail_writing_response ? request_handler_error_code::error : request_handler_error_code::no_error
			};
		}

		auto& request() const { return m_request; }

	private:
		std::unique_ptr<char[]> m_error_message;
		std::string_view m_response_body;
		std::string_view::iterator m_read_offset;
		std::string m_request;
		bool m_rej_req{false};
		bool m_fail_writing_response{false};
	};
}

TESTCASE(west_http_request_processor_process_socket_initial_io_error)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{""}};
	proc.session().connection.enable_read_error();
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::io_error);
}


TESTCASE(west_http_request_processor_process_socket_connection_closed_early_write_no_response_body)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{""}};
	proc.session().connection.client_stop_write();
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::completed);
	EXPECT_EQ(proc.session().connection.output(), "HTTP/1.1 400 Bad request\r\n"
	"Content-Length: 16\r\n"
"\r\n"
"Header truncated");
}

TESTCASE(west_http_request_processor_process_socket_connection_closed_early_write_response_body)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{"This is a test"}};
	proc.session().connection.client_stop_write();
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::completed);
	EXPECT_EQ(proc.session().connection.output(), "HTTP/1.1 400 Bad request\r\n"
	"Content-Length: 16\r\n"
"\r\n"
"Header truncated");
}

TESTCASE(west_http_request_processor_process_socket_connection_closed_while_reading_request_header)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"Content-Length: 469\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis."
"GET /favicon.ico HTTP/1.1\r\nHost: localhost:8000\r\n\r\n"};

	west::http::request_processor proc{socket{}, request_handler{"This is a test"}};
	proc.session().connection.request(request);
	proc.session().connection.read_blocks(2);
	{
		auto const res = proc.socket_is_ready();
		EXPECT_EQ(res, west::http::request_processor_status::more_data_needed);
		EXPECT_EQ(proc.session().request_handler.request().empty(), true);
	}

	proc.session().connection.client_stop_write();

	{
		auto const res = proc.socket_is_ready();
		EXPECT_EQ(res, west::http::request_processor_status::completed);
		EXPECT_EQ(proc.session().request_handler.request().empty(), true);
		EXPECT_EQ(proc.session().connection.output(), "HTTP/1.1 400 Bad request\r\n"
"Content-Length: 16\r\n"
"\r\n"
"Header truncated");
	}
}

TESTCASE(west_http_request_processor_process_socket_connection_closed_while_reading_request_body)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"Content-Length: 469\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis."
"GET /favicon.ico HTTP/1.1\r\nHost: localhost:8000\r\n\r\n"};

	west::http::request_processor proc{socket{}, request_handler{"This is a test"}};
	proc.session().connection.request(request);
	proc.session().connection.read_blocks(8);
	{
		auto const res = proc.socket_is_ready();
		EXPECT_EQ(res, west::http::request_processor_status::more_data_needed);
		EXPECT_EQ(proc.session().request_handler.request().empty(), true);
	}

	{
		auto const res = proc.socket_is_ready();
		EXPECT_EQ(res, west::http::request_processor_status::more_data_needed);
		EXPECT_EQ(proc.session().request_handler.request(),
			"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit sollicit");
	}

	proc.session().connection.client_stop_write();
	{
		auto const res = proc.socket_is_ready();
		EXPECT_EQ(res, west::http::request_processor_status::completed);
		EXPECT_EQ(proc.session().request_handler.request(),
			"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit sollicit");
		EXPECT_EQ(proc.session().connection.output(), "HTTP/1.1 400 Bad request\r\n"
"Content-Length: 40\r\n"
"\r\n"
"Client claims there is more data to read");
	}
}

TESTCASE(west_http_request_processor_process_socket_bad_header_rej_by_fwk)
{
	std::string_view request{"GET / HTTP/1.2\r\n"
"Host: localhost:8000\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{""}};
	proc.session().connection.request(request);
	while(true)
	{
		auto const res = proc.socket_is_ready();
		if(res != west::http::request_processor_status::more_data_needed)
		{
			EXPECT_EQ(res, west::http::request_processor_status::completed);
			EXPECT_EQ(proc.session().connection.server_read_closed(), true);
			EXPECT_EQ(proc.session().connection.output(),
"HTTP/1.1 505 Http version not supported\r\n"
"Content-Length: 46\r\n"
"\r\n"
"This web server only supports HTTP version 1.1");
			break;
		}
		else
		{
			EXPECT_EQ(proc.session().connection.server_read_closed(), false);
		}
	}
}

TESTCASE(west_http_request_processor_process_socket_bad_header_rej_by_app_1)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"Trigger-Rej-By-App: At header validation\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{""}};
	proc.session().connection.request(request);
	while(true)
	{
		auto const res = proc.socket_is_ready();
		if(res != west::http::request_processor_status::more_data_needed)
		{
			EXPECT_EQ(res, west::http::request_processor_status::completed);
			EXPECT_EQ(proc.session().connection.output(),
"HTTP/1.1 418 I am a teapot\r\n"
"Content-Length: 69\r\n"
"\r\n"
"Application requested request to be reject when validating the header");
			EXPECT_EQ(proc.session().connection.server_read_closed(), true);
			break;
		}
		else
		{
			EXPECT_EQ(proc.session().connection.server_read_closed(), false);
		}
	}
}

TESTCASE(west_http_request_processor_process_socket_bad_header_rej_by_app_2)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"Trigger-Rej-By-App: At body competion\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{""}};
	proc.session().connection.request(request);
	while(true)
	{
		auto const res = proc.socket_is_ready();
		if(res != west::http::request_processor_status::more_data_needed)
		{
			EXPECT_EQ(res, west::http::request_processor_status::completed);
			EXPECT_EQ(proc.session().connection.output(),
"HTTP/1.1 422 Unprocessable content\r\n"
"Content-Length: 70\r\n"
"\r\n"
"Application requested request to be reject when the body was processed");
			EXPECT_EQ(proc.session().connection.server_read_closed(), true);
			break;
		}
		else
		{
			EXPECT_EQ(proc.session().connection.server_read_closed(), false);
		}
	}
}

TESTCASE(west_http_request_processor_process_socket_bad_header_fail_writing_response)
{
	std::string_view request{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"Fail-Writing-Response: true\r\n"
"\r\n"};

	west::http::request_processor proc{socket{}, request_handler{"Hello, World"}};
	proc.session().connection.request(request);
	while(true)
	{
		auto const res = proc.socket_is_ready();
		if(res != west::http::request_processor_status::more_data_needed)
		{
			EXPECT_EQ(res, west::http::request_processor_status::application_error);
			EXPECT_EQ(proc.session().connection.output(),
"HTTP/1.1 200 Ok\r\n"
"Content-Length: 12\r\n"
"\r\n");
			EXPECT_EQ(proc.session().connection.server_read_closed(), false);
			break;
		}
		else
		{
			EXPECT_EQ(proc.session().connection.server_read_closed(), false);
		}
	}
}