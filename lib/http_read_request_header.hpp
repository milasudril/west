#ifndef WEST_HTTP_READ_REQUEST_HEADER_HPP
#define WEST_HTTP_READ_REQUEST_HEADER_HPP

#include "./io_interfaces.hpp"
#include "./http_request_handler.hpp"
#include "./http_session_state.hpp"
#include "./http_request_header_parser.hpp"
#include "./utils.hpp"

namespace west::http
{
	class read_request_header
	{
	public:
		read_request_header():m_keep_alive{false}{}

		template<io::data_source Source, request_handler RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
 			Source& source,
			RequestHandler& req_handler);

		std::optional<size_t> get_content_length() const
		{ return m_content_length; }

		bool get_keep_alive() const
		{ return m_keep_alive; }

	private:
		request_header_parser m_req_header_parser;
		std::optional<size_t> m_content_length;
		bool m_keep_alive;
	};
}

template<west::io::data_source Source, west::http::request_handler RequestHandler, size_t BufferSize>
[[nodiscard]] auto west::http::read_request_header::operator()(
	io::buffer_view<char, BufferSize>& buffer,
	Source& source,
	RequestHandler& req_handler)
{
	while(true)
	{
		auto const parse_result = m_req_header_parser.parse(buffer.span_to_read());
		buffer.consume_elements(parse_result.ptr - std::begin(buffer.span_to_read()));
		switch(parse_result.ec)
		{
			case req_header_parser_error_code::completed:
			{
				auto header = m_req_header_parser.take_result();
				if(auto content_length = header.fields.find("content-length");
					 content_length != std::end(header.fields))
				{ m_content_length = to_number<size_t>(content_length->second); }

				if(auto connection = header.fields.find("connection"); connection != std::end(header.fields))
				{ m_keep_alive = (connection->second == "keep-alive" && m_content_length.has_value()); }

				auto res = req_handler.set_header(std::move(header));
				return session_state_response{
					.status = is_client_error(res.http_status) ?
						session_state_status::client_error_detected : session_state_status::completed,
					.http_status = res.http_status,
					.error_message = std::move(res.error_message)
				};
			}

			case req_header_parser_error_code::more_data_needed:
			{
				auto const read_result = source.read(buffer.span_to_write());
				buffer.reset_with_new_length(read_result.bytes_read);

				switch(read_result.ec)
				{
					case io::operation_result::completed:
						// The parser needs more data
						// We do not have any new data
						// This is a client error
						return session_state_response{
							.status = session_state_status::client_error_detected,
							.http_status = status::bad_request,
							.error_message = make_unique_cstr(to_string(parse_result.ec))
						};

					case io::operation_result::more_data_present:
						break;

					case io::operation_result::operation_would_block:
						// Suspend operation until we are waken up again
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
				}

				break;
			}

			default:
				return session_state_response{
					.status = session_state_status::client_error_detected,
					.http_status = status::bad_request,
					.error_message = make_unique_cstr(to_string(parse_result.ec))
				};
		}
	}
}

#endif