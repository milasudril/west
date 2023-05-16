#ifndef WEST_HTTP_REQUEST_HANDLER_HPP
#define WEST_HTTP_REQUEST_HANDLER_HPP

#include "./http_message_header.hpp"

#include <memory>

namespace west::http
{
	struct finalize_state_result
	{
		status http_status{status::ok};
		std::unique_ptr<char[]> error_message;
	};

	template<class T>
	concept error_code = requires(T x)
	{
		{to_string(x)} -> std::convertible_to<char const*>;
		{is_error_indicator(x)} -> std::same_as<bool>;
	};

	template<class T>
	concept process_request_content_result = requires(T x)
	{
		{x.ptr} -> std::convertible_to<char const*>;
		{x.ec} -> error_code;
	};

	template<class T>
	concept request_handler = requires(T x,
		request_header const& req_header,
		std::span<char const> buffer,
		response_header& resp_header)
	{
		{x.finalize_state(req_header)} -> std::same_as<finalize_state_result>;

		{x.process_request_content(buffer)} -> process_request_content_result;

		{x.finalize_state(resp_header)} -> std::same_as<finalize_state_result>;
	};
}

#endif