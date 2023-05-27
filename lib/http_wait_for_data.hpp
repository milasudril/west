#ifndef WEST_HTTP_WAIT_FOR_DATA_HPP
#define WEST_HTTP_WAIT_FOR_DATA_HPP

#include "./io_interfaces.hpp"
#include "./io_adapter.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./http_request_header_parser.hpp"

namespace west::http
{
	class wait_for_data
	{
	public:
		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::wait_for_data::operator()(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	if(std::size(buffer.span_to_read()) != 0)
	{
		return session_state_response{
			.status = session_state_status::completed,
			.state_result = finalize_state_result {
				.http_status = status::ok,
				.error_message = nullptr
			}
		};
	}

	auto const read_result = session.connection.read(buffer.span_to_write());
	buffer.reset_with_new_length(read_result.bytes_read);

	switch(read_result.ec)
	{
		case io::operation_result::completed:
			return session_state_response{
				.status = session_state_status::connection_closed,
				.state_result = finalize_state_result{
					.http_status = status::ok,
					.error_message = nullptr
				}
			};

		case io::operation_result::object_is_still_ready:
			return session_state_response{
				.status = session_state_status::completed,
				.state_result = finalize_state_result{
					.http_status = status::ok,
					.error_message = nullptr
				}
			};

		case io::operation_result::operation_would_block:
			return session_state_response{
				.status = session_state_status::more_data_needed,
				.state_result = finalize_state_result{
					.http_status = status::ok,
					.error_message = nullptr
				}
			};

		case io::operation_result::error:
			return session_state_response{
				.status = session_state_status::io_error,
				.state_result = finalize_state_result{
					.http_status = status::internal_server_error,
					.error_message = make_unique_cstr("I/O error")
				}
			};

		default:
			__builtin_unreachable();
	}
}

#endif