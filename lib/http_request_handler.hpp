#ifndef WEST_HTTP_REQUEST_HANDLER_HPP
#define WEST_HTTP_REQUEST_HANDLER_HPP

#include "./http_message_header.hpp"
#include "./io_adapter.hpp"

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
		{can_continue(x)} -> std::same_as<bool>;
	};

	template<class T>
	concept process_request_content_result = requires(T x)
	{
		{x.bytes_written} -> std::same_as<size_t&>;
		{x.ec} -> error_code;
	};

	template<class T>
	concept read_response_content_result = requires(T x)
	{
		{x.bytes_read} -> std::same_as<size_t&>;
		{x.ec} -> error_code;
	};

	template<class T>
	concept request_handler = requires(T x,
		request_header const& req_header,
		std::span<char const> input_buffer,
		field_map& response_fields,
		std::span<char> output_buffer)
	{
		{x.finalize_state(req_header)} -> std::same_as<finalize_state_result>;

		{x.process_request_content(input_buffer)} -> process_request_content_result;

		{x.finalize_state(response_fields)} -> std::same_as<finalize_state_result>;

		{x.read_response_content(output_buffer)} -> read_response_content_result;
	};
}

#endif