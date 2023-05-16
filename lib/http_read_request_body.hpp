#ifndef WEST_HTTP_READ_REQUEST_BODY_HPP
#define WEST_HTTP_READ_REQUEST_BODY_HPP

#include "./io_interfaces.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./utils.hpp"

namespace west::http
{
	class read_request_body
	{
	public:
		explicit read_request_body(size_t content_length):
			m_content_length{content_length}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		size_t m_content_length;
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::read_request_body::operator()(
	io::buffer_view<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	while(m_content_length != 0)
	{
		auto span_to_read = buffer.span_to_read();
		span_to_read = std::span{std::begin(span_to_read), std::min(std::size(span_to_read), m_content_length)};
		auto const parse_result = session.request_handler.process_request_content(span_to_read);
		auto const bytes_consumed = parse_result.ptr - std::data(span_to_read);
		buffer.consume_elements(bytes_consumed);
		m_content_length -= bytes_consumed;

		if(is_error_indicator(parse_result.ec))
		{
			return session_state_response{
				.status = session_state_status::client_error_detected,
				.http_status = status::bad_request,
				.error_message = make_unique_cstr(to_string(parse_result.ec))
			};
		}

		if(std::size(buffer.span_to_read()) == 0 && m_content_length != 0)
		{
			auto span_to_write = buffer.span_to_write();
			span_to_write = std::span{std::begin(span_to_write), std::min(BufferSize, m_content_length)};
			auto const read_result = session.connection.read(span_to_write);
			buffer.reset_with_new_length(read_result.bytes_read);

			switch(read_result.ec)
			{
				case io::operation_result::completed:
					// Since content length was not zero, we need more data
					// We do not have any new data
					// This is a client error
					return session_state_response{
						.status = session_state_status::client_error_detected,
						.http_status = status::bad_request,
						.error_message = make_unique_cstr("Client claims there is more data to read")
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
	}

	session.response_header.status_line.http_version = version{1, 1};
	auto res = session.request_handler.finalize_state(session.response_header);
	session.response_header.status_line.status_code = res.http_status;
	session.response_header.status_line.reason_phrase = to_string(res.http_status);

	return session_state_response{
		.status = is_client_error(res.http_status) ?
			session_state_status::client_error_detected : session_state_status::completed,
		.http_status = res.http_status,
		.error_message = std::move(res.error_message)
	};
}

#endif