#ifndef WEST_HTTP_WRITE_REQUEST_HEADER_HPP
#define WEST_HTTP_WRITE_REQUEST_HEADER_HPP

#include "./io_interfaces.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./http_response_header_serializer.hpp"

namespace west::http
{
	class write_response_header
	{
	public:
		explicit write_response_header(response_header const& resp_header):
			m_serializer{resp_header}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		response_header_serializer m_serializer;
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::write_response_header::operator()(
	io::buffer_view<char, BufferSize>&,
	session<Source, RequestHandler>&)
{
	puts("Hej");

	return session_state_response{
		.status = session_state_status::completed,
		.http_status = status::ok,
		.error_message = nullptr
	};
}

#endif