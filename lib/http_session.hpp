#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_message_header.hpp"
#include "./http_request_handler.hpp"
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
		enum status http_status;
		std::unique_ptr<char[]> error_message;
	};

	template<class MsgSource>
	auto make_read_response(io::operation_result res,
		MsgSource&& at_conn_close_msg)
	{
		switch(res)
		{
			case io::operation_result::completed:
				return session_state_response{
					.status = session_state_status::client_error_detected,
					.http_status = status::bad_request,
					.error_message = make_unique_cstr(std::forward<MsgSource>(at_conn_close_msg)())
				};

			case io::operation_result::object_is_still_ready:
				return abort<session_state_response>();

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
	}

	inline auto make_write_response(io::operation_result res)
	{
		switch(res)
		{
			case io::operation_result::completed:
				return session_state_response{
					.status = session_state_status::connection_closed,
					.http_status = status::ok,
					.error_message = nullptr
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
	}
}

#endif