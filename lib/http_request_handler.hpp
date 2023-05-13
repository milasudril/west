#ifndef WEST_HTTP_REQUEST_HANDLER_HPP
#define WEST_HTTP_REQUEST_HANDLER_HPP

#include "./http_message_header.hpp"

#include <memory>

namespace west::http
{
	struct header_validation_result
	{
		status http_status{status::ok};
		std::unique_ptr<char[]> error_message;
	};

	template<class T>
	concept request_handler = requires(T x, request_header header)
	{
		{x.set_header(std::move(header))} -> std::same_as<header_validation_result>;
	};
}

#endif