#ifndef WEST_HTTP_REQUEST_STATE_TRANSITIONS_HPP
#define WEST_HTTP_REQUEST_STATE_TRANSITIONS_HPP

#include "./http_read_request_header.hpp"
#include "./http_read_request_body.hpp"
#include "./http_write_response_header.hpp"
#include "./http_write_response_body.hpp"
#include "./http_wait_for_data.hpp"

#include <variant>

namespace west::http
{
	using request_state_holder = std::variant<read_request_header,
		read_request_body,
		write_response_header,
		write_response_body,
		wait_for_data>;



	template<class T>
	struct next_request_state{};

	template<>
	struct next_request_state<read_request_header>
	{ using state_handler = read_request_body; };

	template<>
	struct next_request_state<read_request_body>
	{ using state_handler = write_response_header; };

	template<>
	struct next_request_state<write_response_header>
	{ using state_handler = write_response_body; };

	template<>
	struct next_request_state<write_response_body>
	{ using state_handler = wait_for_data; };

	template<>
	struct next_request_state<wait_for_data>
	{ using state_handler = read_request_header; };



	template<class T>
	struct select_buffer_index{};

	template<>
	struct select_buffer_index<read_request_header>
	{ static constexpr size_t value = 0; };

	template<>
	struct select_buffer_index<read_request_body>
	{ static constexpr size_t value = 0; };

	template<>
	struct select_buffer_index<write_response_header>
	{ static constexpr size_t value = 1; };

	template<>
	struct select_buffer_index<write_response_body>
	{ static constexpr size_t value = 1; };

	template<>
	struct select_buffer_index<wait_for_data>
	{ static constexpr size_t value = 0; };



	template<class T>
	inline auto make_state_handler(request_info const&, response_info const&);

	template<>
	inline auto make_state_handler<read_request_header>(request_info const&, response_info const&)
	{ return read_request_header{}; }

	template<>
	inline auto make_state_handler<read_request_body>(request_info const& request,
		response_info const&)
	{
		auto i = request.header.fields.find("Content-Length");
		if(i == std::end(request.header.fields))
		{ return read_request_body{static_cast<size_t>(0)}; }

		auto const length_conv = to_number<size_t>(i->second);
		return read_request_body{length_conv.value()};
	}

	template<>
	inline auto make_state_handler<write_response_header>(request_info const&,
		response_info const& response)
	{ return write_response_header{response.header}; }

	template<>
	inline auto make_state_handler<write_response_body>(request_info const&,
		response_info const& response)
	{
		assert(!response.header.fields.contains("Transfer-encoding"));

		auto i = response.header.fields.find("Content-Length");
		if(i == std::end(response.header.fields))
		{ return write_response_body{static_cast<size_t>(0)}; }

		auto length_conv = to_number<size_t>(i->second);
		assert(length_conv.has_value());

		return write_response_body{*length_conv};
	}

	template<>
	inline auto make_state_handler<wait_for_data>(request_info const&,
		response_info const&)
	{ return wait_for_data{}; }



	inline auto make_state_handler(request_state_holder const& initial_state,
		request_info const& request,
		response_info const& response)
	{
		return std::visit([&request, response]<class T>(T const&) {
			using next_state_handler = next_request_state<T>::state_handler;
			return request_state_holder{make_state_handler<next_state_handler>(request, response)};
		}, initial_state);
	}
}

#endif