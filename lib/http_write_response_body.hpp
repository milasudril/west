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
		{
			fprintf(stderr, "Server is going to send %zu bytes\n", bytes_to_write);
			fflush(stderr);
		}

		template<io::data_sink Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto socket_is_ready(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		size_t m_bytes_to_write;
	};
}

template<west::io::data_sink Sink, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::write_response_body::socket_is_ready(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Sink, RequestHandler>& session)
{
	return transfer_data(
		[&req_handler = session.request_handler](std::span<char> buffer){
			return req_handler.read_response_content(buffer);
		},
		[&dest = session.connection](std::span<char const> buffer) {
			return dest.write(buffer);
		},
		overload{
			[](auto ec) {
				auto const keep_going = can_continue(ec);
				return session_state_response{
					.status = keep_going ?
						session_state_status::more_data_needed :
						session_state_status::write_response_failed,
					.state_result = finalize_state_result {
						.http_status = keep_going? status::ok : status::internal_server_error,
						.error_message = keep_going? nullptr : make_unique_cstr(to_string(ec))
					}
				};
			},
			[](io::operation_result res){ return make_write_response(res); },
			[]() {
				return session_state_response{
					.status = session_state_status::completed,
					.state_result = finalize_state_result {
						.http_status = status::ok,
						.error_message = nullptr
					}
				};
			}
		},
		buffer,
		m_bytes_to_write
	);
}

#endif
