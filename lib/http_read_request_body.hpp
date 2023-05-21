#ifndef WEST_HTTP_READ_REQUEST_BODY_HPP
#define WEST_HTTP_READ_REQUEST_BODY_HPP

#include "./io_interfaces.hpp"
#include "./io_adapter.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./utils.hpp"

namespace west::http
{
	class read_request_body
	{
	public:
		explicit read_request_body(size_t content_length):
			m_content_length{content_length}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		size_t m_content_length;
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::read_request_body::operator()(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	return transfer_data(
		[&src = session.connection](std::span<char> buffer){
			return src.read(buffer);
		},
		[&req_handler = session.request_handler](std::span<char const> buffer){
			return req_handler.process_request_content(buffer);
		},
		overload{
			[](io::operation_result res){
				return make_read_response(res, [](){return "Client claims there is more data to read";});
			},
			[](auto ec){
				auto const keep_going = can_continue(ec);
				return session_state_response{
					.status = keep_going ?
						session_state_status::more_data_needed :
						session_state_status::client_error_detected,
					.http_status = keep_going? status::ok : status::bad_request,
					.error_message = keep_going? nullptr : make_unique_cstr(to_string(ec))
				};
			},
			[&session](){
				session.response_header = response_header{};
				session.response_header.status_line.http_version = version{1, 1};
				auto res = session.request_handler.finalize_state(session.response_header.fields);
				session.response_header.status_line.status_code = res.http_status;
				session.response_header.status_line.reason_phrase = to_string(res.http_status);

				return session_state_response{
					.status = is_client_error(res.http_status) ?
						session_state_status::client_error_detected : session_state_status::completed,
					.http_status = res.http_status,
					.error_message = std::move(res.error_message)
				};
			}
		},
	buffer,
	m_content_length);
}

#endif