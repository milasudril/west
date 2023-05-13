#ifndef WEST_HTTP_SESSION_STATE_HPP
#define WEST_HTTP_SESSION_STATE_HPP

#include "./http_message_header.hpp"

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

	struct session_state_response
	{
		session_state_status status;
		enum status http_status;
		std::unique_ptr<char[]> error_message;
	};
}

#endif