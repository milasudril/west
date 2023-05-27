#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_message_header.hpp"
#include "./http_request_handler.hpp"
#include "./io_interfaces.hpp"
#include "./utils.hpp"

#include <memory>

namespace west::http
{
	enum class session_state_status
	{
		completed,
		connection_closed,
		more_data_needed,
		client_error_detected,
		write_response_failed,
		io_error
	};

	template<class Socket, class RequestHandler>
	struct session
	{
		Socket connection;
		RequestHandler request_handler;
		class request_header request_header;
		class response_header response_header;
	};

	struct session_state_response
	{
		session_state_status status;
		finalize_state_result state_result;
	};

	template<class MsgSource>
	auto make_read_response(io::operation_result res,
		MsgSource&& at_conn_close_msg)
	{
		switch(res)
		{

			case io::operation_result::completed:
				// NOTE: Client have closed its "write" end of the socket. Its may "read" end may still
				//       be open, and thus, it may be possible to respond with a HTTP 400 message.
				return session_state_response{
					.status = session_state_status::client_error_detected,
					.state_result = finalize_state_result{
						.http_status = status::bad_request,
						.error_message = make_unique_cstr(std::forward<MsgSource>(at_conn_close_msg)())
					}
				};

			// GCOVR_EXCL_START
			case io::operation_result::object_is_still_ready:
				return abort<session_state_response>();
			// GCOVR_EXCL_STOP

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
			// GCOVR_EXCL_START
			default:
				__builtin_unreachable();  // LCOV_EXCL_LINE
			// GCOVR_EXCL_STOP
		}
	}

	inline auto make_write_response(io::operation_result res)
	{
		switch(res)
		{
			case io::operation_result::completed:
				return session_state_response{
					.status = session_state_status::connection_closed,
					.state_result  = finalize_state_result {
						.http_status = status::ok,
						.error_message = nullptr
					}
				};

			// GCOVR_EXCL_START
			case io::operation_result::object_is_still_ready:
				abort();
			// GCOVR_EXCL_STOP

			case io::operation_result::operation_would_block:
				return session_state_response{
					.status = session_state_status::more_data_needed,
					.state_result = finalize_state_result {
						.http_status = status::ok,
						.error_message = nullptr
					}
				};

			case io::operation_result::error:
				return session_state_response{
					.status = session_state_status::io_error,
					.state_result = finalize_state_result {
						.http_status = status::internal_server_error,
						.error_message = make_unique_cstr("I/O error")
					}
				};
			// GCOVR_EXCL_START
			default:
				__builtin_unreachable();
 			// GCOVR_EXCL_STOP
		}
	}
}

#endif