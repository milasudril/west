#ifndef WEST_HTTP_WRITE_RESPONSE_HEADER_HPP
#define WEST_HTTP_WRITE_RESPONSE_HEADER_HPP

#include "./io_interfaces.hpp"
#include "./io_adapter.hpp"
#include "./http_request_handler.hpp"
#include "./http_session.hpp"
#include "./http_response_header_serializer.hpp"

#include <limits>

namespace west::http
{
	class write_response_header
	{
	public:
		explicit write_response_header(response_header const& resp_header):
			m_serializer{resp_header},
			m_saved_serializer_ec{resp_header_serializer_error_code::more_data_needed}
		{}

		template<io::data_source Source, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Source, RequestHandler>& session);

	private:
		response_header_serializer m_serializer;
		resp_header_serializer_error_code m_saved_serializer_ec;
	};
}

template<west::io::data_source Source, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::write_response_header::operator()(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Source, RequestHandler>& session)
{
	auto max_length = std::numeric_limits<size_t>::max();  // No limit in how much data we can write
	return transfer_data(
		[this](std::span<char> buffer){
			struct read_result
			{
				size_t bytes_read;
				resp_header_serializer_error_code ec;
			};

			auto res = m_serializer.serialize(buffer);
			return read_result{static_cast<size_t>(res.ptr - std::data(buffer)), res.ec};
		},
		[&dest = session.connection](std::span<char const> buffer){
			return dest.write(buffer);
		},
		overload{
			[&saved_ec = m_saved_serializer_ec,
			 &buffer = std::as_const(buffer)](resp_header_serializer_error_code ec){
				assert(ec == resp_header_serializer_error_code::completed);
				saved_ec = ec;
				return session_state_response{
					.status = std::size(buffer.span_to_read()) == 0?
						session_state_status::completed : session_state_status::more_data_needed,
					.http_status = status::ok,
					.error_message = nullptr
				};
			},
			[&saved_ec = m_saved_serializer_ec](io::operation_result res){
				switch(res)
				{
					case io::operation_result::completed:
					{
						if(saved_ec == resp_header_serializer_error_code::completed) [[likely]]
						{
							// Return OK, since we do not know anything about the response body
							return session_state_response{
								.status = session_state_status::completed,
								.http_status = status::ok,
								.error_message = nullptr
							};
						}
						else
						{
							return session_state_response{
								.status = session_state_status::io_error,
								.http_status = status::internal_server_error,
								.error_message = make_unique_cstr("I/O error")
							};
						}
					}

					case io::operation_result::object_is_still_ready:
						abort();

					case io::operation_result::operation_would_block:
						return session_state_response{
							.status = session_state_status::more_data_needed,
							.http_status = status::ok,
							.error_message = nullptr
						};

					case io::operation_result::error:
						return session_state_response{
							.status = session_state_status::io_error,
							.http_status = status::internal_server_error,
							.error_message = make_unique_cstr("I/O error")
						};
					default:
						__builtin_unreachable();
				}
			},
			[](){return abort<session_state_response>();},
		},
		buffer,
		max_length
	);
}

#endif