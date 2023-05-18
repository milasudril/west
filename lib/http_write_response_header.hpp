#ifndef WEST_HTTP_WRITE_REQUEST_HEADER_HPP
#define WEST_HTTP_WRITE_REQUEST_HEADER_HPP

#include "./io_interfaces.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./http_response_header_serializer.hpp"

namespace west::http
{
	class write_response_header
	{
	public:
		explicit write_response_header(response_header const& resp_header):
			m_serializer{resp_header}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		response_header_serializer m_serializer;
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::write_response_header::operator()(
	io::buffer_view<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	while(true)
	{
		auto write_res = session.connection.write(buffer.span_to_read());
		buffer.consume_elements(write_res.bytes_written);
		if(std::size(buffer.span_to_read()) == 0)
		{
			auto read_result = m_serializer.serialize(buffer.span_to_write());
			buffer.reset_with_new_length(read_result.ptr - std::data(buffer.span_to_write()));

			switch(read_result.ec)
			{
				case resp_header_serializer_error_code::more_data_needed:
					break;

				case resp_header_serializer_error_code::completed:
					//TODO: Should flush in next state
					if(std::size(buffer.span_to_read()) == 0)
					{
						return session_state_response{
							.status = session_state_status::completed,
							.http_status = status::ok,
							.error_message = nullptr
						};
					}
			}
		}

		switch(write_res.ec)
		{
			case io::operation_result::completed:
			{
				if(std::size(buffer.span_to_read()) == 0) [[likely]]
				{
					// Return OK, since we do not know anything about the response body
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
						.error_message = make_unique_cstr("I/O error")
					};
				}
			}

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

#endif