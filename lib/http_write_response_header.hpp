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
			m_serializer{resp_header}
		{}

		template<io::data_sink Sink, class RequestHandler, size_t BufferSize>
		[[nodiscard]] auto socket_is_ready(io_adapter::buffer_span<char, BufferSize>& buffer,
			session<Sink, RequestHandler>& session);

	private:
		response_header_serializer m_serializer;
	};
}

template<>
struct west::io_adapter::error_code_checker<west::http::resp_header_serializer_error_code>
{
	constexpr bool operator()(http::resp_header_serializer_error_code) const
	{ return false; }
};

template<west::io::data_sink Sink, class RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::write_response_header::socket_is_ready(
	io_adapter::buffer_span<char, BufferSize>& buffer,
	session<Sink, RequestHandler>& session)
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
			[&buffer = std::as_const(buffer)](resp_header_serializer_error_code, auto&&...){
				assert(std::size(buffer.span_to_read()) == 0);
				return session_state_response{
					.status = session_state_status::completed,
					.state_result = finalize_state_result {
						.http_status = status::ok,
						.error_message = nullptr
					}
				};
			},
			[](io::operation_result res, auto&&...){ return make_write_response(res); },
		// GCOVR_EXCL_START
			[](){return abort<session_state_response>();}
		// GCOVR_EXCL_STOP
		},
		buffer,
		max_length
	);
}

#endif