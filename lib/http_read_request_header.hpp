#ifndef WEST_HTTP_READ_REQUEST_HEADER_HPP
#define WEST_HTTP_READ_REQUEST_HEADER_HPP

#include "./io_interfaces.hpp"
#include "./io_adapter.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./http_request_header_parser.hpp"

namespace west::http
{
	class read_request_header
	{
	public:
		explicit read_request_header(size_t max_header_size = 65536):
			m_bytes_to_read{max_header_size}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto socket_is_ready(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		request_header_parser m_req_header_parser;
		size_t m_bytes_to_read;
	};
}

template<>
struct west::io_adapter::error_code_checker<west::http::req_header_parser_error_code>
{
	constexpr bool operator()(http::req_header_parser_error_code ec) const
	{
		// NOTE: In order to force io_adapter::transfer_data to return when
		//       header parsing has completed, `completed` is treated as an
		//       error condition.
		//
		return ec != http::req_header_parser_error_code::more_data_needed;
	}
};

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::read_request_header::socket_is_ready(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	return transfer_data(
		[&src = session.connection](std::span<char> buffer){
			return src.read(buffer);
		},
		[&req_header_parser = m_req_header_parser](std::span<char const> buffer){
			struct parse_result
			{
				size_t bytes_written;
				req_header_parser_error_code ec;
			};

			auto res = req_header_parser.parse(buffer);
			return parse_result{static_cast<size_t>(res.ptr - std::begin(buffer)), res.ec};
		},
		overload{
			[](io::operation_result res){
				return make_read_response(res, [](){
					return to_string(req_header_parser_error_code::more_data_needed);
				});
			},
			[&req_header_parser = m_req_header_parser, &session](req_header_parser_error_code ec){
				switch(ec)
				{
				// GCOVR_EXCL_START
					case req_header_parser_error_code::more_data_needed:
						abort();
				// GCOVR_EXCL_STOP
					case req_header_parser_error_code::completed:
					{
						auto header = req_header_parser.take_result();
						if(header.request_line.http_version != version{1, 1})
						{
							return session_state_response{
								.status = session_state_status::client_error_detected,
								.state_result = finalize_state_result{
									.http_status = status::http_version_not_supported,
									.error_message = make_unique_cstr("This web server only supports HTTP version 1.1")
								}
							};
						}

						// TODO: Introduce limit on content-length here
						auto const content_length = get_content_length(header);
						if(!content_length.has_value())
						{
							return session_state_response{
								.status = session_state_status::client_error_detected,
								.state_result = finalize_state_result{
									.http_status = status::bad_request,
									.error_message = make_unique_cstr("Bad content-length")
								}
							};
						}

						auto res = session.request_handler.finalize_state(header);
						session.request_info.header = std::move(header);
						session.request_info.content_length = *content_length;
						auto const saved_http_status = res.http_status;
						return session_state_response{
							.status = is_error(saved_http_status) ?
								session_state_status::client_error_detected : session_state_status::completed,
							.state_result = std::move(res)
						};
					}

					case req_header_parser_error_code::bad_request_method:
					case req_header_parser_error_code::bad_request_target:
					case req_header_parser_error_code::wrong_protocol:
					case req_header_parser_error_code::bad_protocol_version:
					case req_header_parser_error_code::expected_linefeed:
					case req_header_parser_error_code::bad_field_name:
					case req_header_parser_error_code::bad_field_value:
					default:
						return session_state_response{
							.status = session_state_status::client_error_detected,
							.state_result = finalize_state_result{
								.http_status = status::bad_request,
								.error_message = make_unique_cstr(to_string(ec))
							}
						};
				}
			},
			[](){
				return session_state_response{
					.status = session_state_status::client_error_detected,
					.state_result = finalize_state_result{
						.http_status = status::request_entity_too_large,
						.error_message = make_unique_cstr(to_string(req_header_parser_error_code::more_data_needed))
					}
				};
			}
		},
		buffer,
		m_bytes_to_read
	);
}

#endif
