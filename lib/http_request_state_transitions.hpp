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
	// To be moved to different file
	class write_error_response
	{
	public:
		template<class T, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>&, T const&)
		{
			return session_state_response{
				.status = session_state_status::io_error,
				.http_status = status::not_implemented,
				.error_message = make_unique_cstr("Not implemented")
			};
		}
	};

	using request_state_holder = std::variant<read_request_header,
		read_request_body,
		write_response_header,
		write_response_body,
		wait_for_data,
		write_error_response>;

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
	{ using state_handler = read_request_header; };

	template<>
	struct next_request_state<wait_for_data>
	{ using state_handler = read_request_header; };

	template<>
	struct next_request_state<write_error_response>
	{ using state_handler = read_request_header; };



	template<class T>
	inline auto make_state_handler(request_header const&, size_t, response_header const&);

	template<>
	inline auto make_state_handler<read_request_header>(request_header const&,
		size_t current_buffer_size,
		response_header const&)
	{
		if(current_buffer_size == 0)
		{ return request_state_holder{wait_for_data{}}; }
		return request_state_holder{read_request_header{}};
	}

	template<>
	inline auto make_state_handler<read_request_body>(request_header const& req_header,
		size_t,
		response_header const&)
	{
		auto i = req_header.fields.find("Content-Length");
		if(i == std::end(req_header.fields))
		{ return request_state_holder{read_request_body{static_cast<size_t>(0)}}; }

		auto length_conv = to_number<size_t>(i->second);
		if(!length_conv.has_value())
		{ return request_state_holder{write_error_response{}}; }

		return request_state_holder{read_request_body{*length_conv}};
	}

	template<>
	inline auto make_state_handler<write_response_header>(request_header const&,
		size_t,
		response_header const& resp_header)
	{
		return write_response_header{resp_header};
	}

	template<>
	inline auto make_state_handler<write_response_body>(request_header const&,
		size_t,
		response_header const& resp_header)
	{
		assert(!resp_header.fields.contains("Transfer-encoding"));

		auto i = resp_header.fields.find("Content-Length");
		if(i == std::end(resp_header.fields))
		{ return request_state_holder{wait_for_data{}}; }

		auto length_conv = to_number<size_t>(i->second);
		assert(length_conv.has_value());

		return request_state_holder{write_response_body{*length_conv}};
	}

	template<>
	inline auto make_state_handler<write_error_response>(request_header const&,
		size_t,
		response_header const&)
	{ return write_error_response{}; }



	inline auto make_state_handler(request_state_holder const& initial_state,
		request_header const& req_header,
		size_t current_buffer_size,
		response_header const& resp_header)
	{
		return std::visit([&req_header, current_buffer_size, resp_header]<class T>(T const&) {
			using next_state_handler = next_request_state<T>::state_handler;
			return request_state_holder{
				make_state_handler<next_state_handler>(req_header, current_buffer_size, resp_header)
			};
		}, initial_state);
	}
}

#endif