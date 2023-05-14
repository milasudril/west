#ifndef WEST_HTTP_SESSION_STATE_HPP
#define WEST_HTTP_SESSION_STATE_HPP

#include "./http_message_header.hpp"
#include "./http_request_handler.hpp"

#include <memory>

namespace west::http
{
	enum class session_state_status
	{
		completed,
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
		size_t req_content_length{0};
		bool conn_keep_alive{true};
	};

	struct session_state_response
	{
		session_state_status status;
		enum status http_status;
		std::unique_ptr<char[]> error_message;
	};
}

#endif