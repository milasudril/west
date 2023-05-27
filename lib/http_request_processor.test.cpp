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

		auto write(std::span<char const>)
		{
			return west::io::write_result{
				.bytes_written = 0,
				.ec = west::io::operation_result::error
			};
		}

		void stop_reading()
		{ m_endpoint_status |= SERVER_READ_CLOSED; }

		void client_stop_read()
		{ m_endpoint_status |= CLIENT_READ_CLOSED; }

		void client_stop_write()
		{ m_endpoint_status |= CLIENT_WRITE_CLOSED; }

	private:
		uint32_t m_endpoint_status{0};
		static constexpr uint32_t CLIENT_READ_CLOSED{1};
		static constexpr uint32_t CLIENT_WRITE_CLOSED{2};
		static constexpr uint32_t SERVER_READ_CLOSED{4};
		static constexpr uint32_t SERVER_WRITE_CLOSED{8};
	};



	enum class request_handler_error_code{};

	constexpr bool is_error_indicator(request_handler_error_code)
	{	return true; }

	constexpr bool can_continue(request_handler_error_code)
	{ return false; }

	constexpr char const* to_string(request_handler_error_code)
	{ return "Foobar"; }

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
		auto finalize_state(west::http::request_header const&)
		{
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::bad_request;
			validation_result.error_message = west::make_unique_cstr("Invalid request header");
			return validation_result;
		}


		auto finalize_state(west::http::field_map&)
		{
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::bad_request;
			validation_result.error_message = west::make_unique_cstr("Invalid response body");
			return validation_result;
		}

		auto process_request_content(std::span<char const>)
		{
			return request_handler_write_result{};
		}

		auto read_response_content(std::span<char>)
		{
			return request_handler_read_result{};
		}
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

	west::http::request_processor proc{socket{}, request_handler{}};
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::io_error);
}

TESTCASE(west_http_request_processor_process_socket_connection_closed_early)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis."};

	west::http::request_processor proc{socket{}, request_handler{}};
	proc.session().connection.client_stop_write();
	auto const res = proc.socket_is_ready();
	EXPECT_EQ(res, west::http::request_processor_status::io_error);
}