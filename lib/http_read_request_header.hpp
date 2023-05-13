#ifndef WEST_HTTP_READ_REQUEST_HEADER_HPP
#define WEST_HTTP_READ_REQUEST_HEADER_HPP

#include "./io_interfaces.hpp"
#include "./http_request_handler.hpp"
#include "./http_session_state.hpp"
#include "./http_request_header_parser.hpp"
#include "./utils.hpp"

namespace west::http
{
	class read_request_header
	{
	public:
		template<io::socket Socket, request_handler RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
			Socket const& socket,
			RequestHandler& req_handler);

	private:
		request_header_parser m_req_header_parser;
		req_header_parser_error_code m_saved_ec;
	};
}

template<west::io::socket Socket, west::http::request_handler RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::read_request_header::operator()(
	io::buffer_view<char, BufferSize>& buffer,
	Socket const& socket,
	RequestHandler& req_handler)
{
	while(true)
	{
		auto const parse_result = m_req_header_parser.parse(buffer.span_to_read());
		buffer.consume_until(parse_result.ptr);
		switch(parse_result.ec)
		{
			case req_header_parser_error_code::completed:
			{
				auto res = req_handler.set_header(m_req_header_parser.take_result());
				return session_state_response{
					.status = session_state_status::completed,
					.http_status = res.http_status,
					.error_message = std::move(res.error_message)
				};
			}

			case req_header_parser_error_code::more_data_needed:
			{
				auto const read_result = socket.read(buffer.span_to_write());
				buffer.reset_with_new_end(read_result.ptr);

				switch(read_result.ec)
				{
					case io::operation_result::completed:
						// The parser needs more data
						// We do not have any new data
						// This is a client error
						return session_state_response{
							.status = session_state_status::client_error_detected,
							.http_status = status::bad_request,
							.error_message = make_unique_cstr(to_string(parse_result.ec))
						};

					case io::operation_result::more_data_present:
						break;

					case io::operation_result::operation_would_block:
						// Suspend operation until we are waken up again
						return session_state_response{
							.status = session_state_status::more_data_needed,
							.http_status = status::ok,
							.error_message = nullptr
						};

					case io::operation_result::error:
						return session_state_response{
							.status = session_state_status::io_error,
							.http_status = status::internal_server_error,
							.error_message = make_unique_cstr("I/O error")
						};
				}
			}

			default:
				return session_state_response{
					.status = session_state_status::client_error_detected,
					.http_status = status::bad_request,
					.error_message = make_unique_cstr(to_string(parse_result.ec))
				};
		}
	}
}

#endif