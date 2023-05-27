//@	{"target":{"name":"http_request_processor.test"}}

#include "./http_request_processor.hpp"

#include <testfwk/testfwk.hpp>
#include <random>

namespace
{
	class socket
	{
	public:
		auto read(std::span<char>)
		{
			if((m_endpoint_status & CLIENT_WRITE_CLOSED) || (m_endpoint_status & SERVER_READ_CLOSED))
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::completed
				};
			}
			else
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::error
				};
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

	private:
		uint32_t m_endpoint_status{0};
		static constexpr uint32_t CLIENT_READ_CLOSED{1};
		static constexpr uint32_t CLIENT_WRITE_CLOSED{2};
		static constexpr uint32_t SERVER_READ_CLOSED{4};
		static constexpr uint32_t SERVER_WRITE_CLOSED{8};
		static constexpr uint32_t READ_TRIGGERS_ERROR{16};
		static constexpr uint32_t WRITE_TRIGGERS_ERROR{32};

		std::string m_output;
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

		auto finalize_state(west::http::request_header const&)
		{
			puts("Finalize read header");
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::bad_request;
			validation_result.error_message = west::make_unique_cstr("Invalid request header");
			return validation_result;
		}

		auto finalize_state(west::http::field_map& fields)
		{
			puts("Finalize read body");
			west::http::finalize_state_result validation_result;
			fields.append("Content-Length", std::to_string(std::size(m_response_body)));
			validation_result.http_status = west::http::status::i_am_a_teapot;
			validation_result.error_message = west::make_unique_cstr("Invalid request body");
			return validation_result;
		}

		void finalize_state(west::http::field_map& fields, west::http::session_state_response&& res)
		{
			m_error_message = std::move(res.error_message);
			m_response_body = std::string_view{m_error_message.get()};
			m_read_offset = std::begin(m_response_body);
			fields.append("Content-Length", std::to_string(std::size(m_response_body)));
		}

		auto process_request_content(std::span<char const>)
		{
			return request_handler_write_result{};
		}

		auto read_response_content(std::span<char> buffer)
		{
			auto const n_bytes_left = static_cast<size_t>(std::end(m_response_body) - m_read_offset);
			auto const bytes_to_write = std::min(std::size(buffer), n_bytes_left);
			std::copy_n(m_read_offset, bytes_to_write, std::begin(buffer));
			m_read_offset += bytes_to_write;

			return request_handler_read_result{
				bytes_to_write,
				request_handler_error_code::no_error
			};
		}

	private:
		std::unique_ptr<char[]> m_error_message;
		std::string_view m_response_body;
		std::string_view::iterator m_read_offset;
	};
}

TESTCASE(west_http_request_processor_process_socket_initial_io_error)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis."};

	west::http::request_processor proc{socket{}, request_handler{""}};
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::io_error);
}

TESTCASE(west_http_request_processor_process_socket_connection_closed_early_write_no_response_body)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis."};

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
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis."};

	west::http::request_processor proc{socket{}, request_handler{"This is a test"}};
	proc.session().connection.client_stop_write();
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::completed);
	EXPECT_EQ(proc.session().connection.output(), "HTTP/1.1 400 Bad request\r\n"
	"Content-Length: 16\r\n"
"\r\n"
"Header truncated");
}