#ifndef WEST_HTTP_REQUEST_STATE_TRANSITIONS_HPP
#define WEST_HTTP_REQUEST_STATE_TRANSITIONS_HPP

#include "./http_read_request_header.hpp"
#include "./http_read_request_body.hpp"

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


	using request_state_holder = std::variant<read_request_header, read_request_body, write_error_response>;

	template<class T>
	struct next_request_state{};

	template<>
	struct next_request_state<read_request_header>
	{ using state_handler = read_request_body; };

	template<>
	struct next_request_state<read_request_body>
	{ using state_handler = read_request_header; };

	template<>
	struct next_request_state<write_error_response>
	{ using state_handler = read_request_header; };



	template<class T>
	inline auto make_state_handler(request_header const&);

	template<>
	inline auto make_state_handler<read_request_header>(request_header const&)
	{ return read_request_header{}; }

	template<>
	inline auto make_state_handler<write_error_response>(request_header const&)
	{ return write_error_response{}; }



	template<>
	inline auto make_state_handler<read_request_body>(request_header const& req_header)
	{
		auto i = req_header.fields.find("Content-Length");
		if(i == std::end(req_header.fields))
		{ return request_state_holder{read_request_body{static_cast<size_t>(0)}}; }

		auto length_conv = to_number<size_t>(i->second);
		if(!length_conv.has_value())
		{ return request_state_holder{write_error_response{}}; }

		return request_state_holder{read_request_body{*length_conv}};
	}

	inline auto make_state_handler(request_state_holder const& initial_state, request_header const& req_header)
	{
		return std::visit([&req_header]<class T>(T const&) {
			using next_state_handler = next_request_state<T>::state_handler;
			return request_state_holder{make_state_handler<next_state_handler>(req_header)};
		}, initial_state);
	}
}

#endif