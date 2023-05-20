#ifndef WEST_HTTP_WRITE_RESPONSE_BODY_HPP
#define WEST_HTTP_WRITE_RESPONSE_BODY_HPP

#include "./io_interfaces.hpp"
#include "./io_adapter.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"

namespace west::http
{
	class write_response_body
	{
	public:
		explicit write_response_body(size_t bytes_to_write):
			m_bytes_to_write{bytes_to_write}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		size_t m_bytes_to_write;
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::write_response_body::operator()(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	while(true)
	{
		auto span_to_read = buffer.span_to_read();
		span_to_read = std::span{
				std::begin(span_to_read),
				std::min(m_bytes_to_write, std::size(span_to_read))
		};

		auto write_res = session.connection.write(span_to_read);
		buffer.consume_elements(write_res.bytes_written);

		m_bytes_to_write -= write_res.bytes_written;

		if(std::size(buffer.span_to_read()) == 0)
		{
			auto span_to_write = buffer.span_to_write();
			span_to_write = std::span{
				std::begin(span_to_write),
				std::min(m_bytes_to_write, std::size(span_to_write))
			};

			auto read_result = session.request_handler.read_response_content(span_to_write);
			buffer.reset_with_new_length(read_result.ptr - std::data(span_to_write));
			if(should_return(read_result.ec) ||
				(m_bytes_to_write != 0 && std::size(buffer.span_to_read()) == 0))
			{
				return session_state_response{
					.status = session_state_status::io_error,
					.http_status = status::internal_server_error,
					.error_message = make_unique_cstr("Request handler has no data to deliver")
				};
			}

			if(std::size(buffer.span_to_read()) == 0)
			{
				return session_state_response{
					.status = session_state_status::completed,
					.http_status = status::ok,
					.error_message = nullptr
				};
			}
		}

		switch(write_res.ec)
		{
			case io::operation_result::completed:
			{
				if(std::size(buffer.span_to_read()) == 0 && m_bytes_to_write == 0) [[likely]]
				{
					return session_state_response{
						.status = session_state_status::completed,
						.http_status = status::ok,
						.error_message = nullptr
					};
				}
				else
				{
					return session_state_response{
						.status = session_state_status::io_error,
						.http_status = status::internal_server_error,
						.error_message = make_unique_cstr("Client disconnected")
					};
				}
			}

			case io::operation_result::object_is_still_ready:
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

#endif