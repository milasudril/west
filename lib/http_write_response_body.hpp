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
						session_state_status::client_error_detected,
					.http_status = keep_going? status::ok : status::bad_request,
					.error_message = keep_going? nullptr : make_unique_cstr(to_string(ec))
				};
			},
			[](io::operation_result res){
				switch(res)
				{
					case io::operation_result::completed:
						return session_state_response{
							.status = session_state_status::client_error_detected,
							.http_status = status::bad_request,
							.error_message = make_unique_cstr("Client claims there is more data to read")
						};
					case io::operation_result::object_is_still_ready:
						abort();
					case io::operation_result::operation_would_block:
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
					default:
						__builtin_unreachable();
				}
			},
			[]() {
				return session_state_response{
					.status = session_state_status::completed,
					.http_status = status::ok,
					.error_message = nullptr
				};
			}
		},
		buffer,
		m_bytes_to_write
	);
}

#endif