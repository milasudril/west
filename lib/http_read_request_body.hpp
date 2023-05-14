#ifndef WEST_HTTP_READ_REQUEST_BODY_HPP
#define WEST_HTTP_READ_REQUEST_BODY_HPP

#include "./io_interfaces.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"

namespace west::http
{
	class read_request_body
	{
	public:
		explicit read_request_body(size_t content_length):
			m_content_length{content_length}
		{}

		template<io::data_source Source, request_handler RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		size_t m_content_length;
	};
}

template<west::io::data_source Source, west::http::request_handler RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::read_request_body::operator()(
	io::buffer_view<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	while(true)
	{
		auto const parse_result = session.request_handler.process_request_content(buffer.span_to_read());
		buffer.consume_elements(parse_result.ptr - std::begin(buffer.span_to_read()));
		switch(parse_result.ec)
		{
			case request_handler_error_code::completed:
			{
				auto const res = session.request_handler.finalize_request_content_processing();
				return session_state_response{
					.status = is_client_error(res.http_status) ?
						session_state_status::client_error_detected : session_state_status::completed,
					.http_status = res.http_status,
					.error_message = std::move(res.error_message)
				};
			}

			case request_handler_error_code::more_data_needed:
			{
				auto span_to_write = buffer.span_to_write();
				span_to_write = std::span{std::begin(span_to_write), std::max(BufferSize, m_content_length)};
				auto const read_result = session.connection.read(span_to_write);
				m_content_length -= read_result.bytes_read;
				buffer.reset_with_new_length(read_result.bytes_read);

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

				break;
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