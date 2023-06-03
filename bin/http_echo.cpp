//@	{"target":{"name": "http_echo.o"}}

#include "lib/io_inet_server_socket.hpp"
#include "lib/service_registry.hpp"
#include "lib/http_request_handler.hpp"
#include "lib/http_request_processor.hpp"

#include <string>

namespace
{
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
	
	
	class my_request_handler
	{
	public:
		auto finalize_state(west::http::request_header const& header)
		{
			puts("Finalize requset header");
			fflush(stdout);
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = nullptr;

			m_response_body = header.request_line.method.value();
			m_response_body
				.append(" ")
				.append(header.request_line.request_target.value())
				.append(" ")
				.append("HTTP/")
				.append(to_string(header.request_line.http_version))
				.append("\r\n");

			for(auto& item : header.fields)
			{ m_response_body.append(item.first.value()).append(": ").append(item.second).append("\r\n"); }

			m_response_body.append("\r\n");

			return validation_result;
		}

		auto finalize_state(west::http::field_map& fields)
		{
			puts("Finalize read body");
			fflush(stdout);
			fields.append("Content-Length", std::to_string(std::size(m_response_body)))
				.append("Content-Type", "text/plain");
				
			m_read_offset = std::begin(m_response_body);

			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = nullptr;
			return validation_result;
		}

		void finalize_state(west::http::field_map& fields, west::http::finalize_state_result&& res)
		{
			puts("Error");
			fflush(stdout);
			m_error_message = std::move(res.error_message);
			m_response_body = std::string{m_error_message.get()};
			m_read_offset = std::begin(m_response_body);
			fields.append("Content-Length", std::to_string(std::size(m_response_body)));
		}

		auto process_request_content(std::span<char const> buffer)
		{
			puts("Process request content");
			fflush(stdout);
			m_response_body.insert(std::end(m_response_body), std::begin(buffer), std::end(buffer));
			return request_handler_write_result{
				.bytes_written = std::size(buffer),
				.ec = request_handler_error_code::no_error
			};
		}

		auto read_response_content(std::span<char> buffer)
		{
			puts("Read response content");
			fflush(stdout);
			auto const n_bytes_left = static_cast<size_t>(std::end(m_response_body) - m_read_offset);
			auto const bytes_to_read = std::min(std::size(buffer), n_bytes_left);
			std::copy_n(m_read_offset, bytes_to_read, std::begin(buffer));
			m_read_offset += bytes_to_read;

			return request_handler_read_result{
				bytes_to_read,
				request_handler_error_code::no_error
			};
		}

	private:
		std::unique_ptr<char[]> m_error_message;
		std::string m_response_body;
		std::string::iterator m_read_offset;
	};
	
	struct http_session_factory
	{
		auto create_session(west::io::inet_connection&& connection)
		{ return west::http::request_processor{std::move(connection), my_request_handler{}}; }
	};
}

int main()
{
	west::io::inet_address address{"127.0.0.1"};
	west::io::inet_server_socket server_socket{
		address,
		std::ranges::iota_view{49152, 65536},
		128
	};
	printf("Server is listening on port %u\n", server_socket.port());
	west::service_registry{}
		.enroll(std::move(server_socket), http_session_factory{})
		.process_events();
}
