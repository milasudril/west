#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_request_header_parser.hpp"
#include "./http_response_header_serializer.hpp"
#include "./io_interfaces.hpp"

#include <span>
#include <memory>
#include <cstring>

namespace west::http
{
	struct header_validation_result
	{
		status http_status;
		std::unique_ptr<char[]> error_message;
	};

	template<class T>
	concept request_handler = requires(T x, request_header header)
	{
		{x.set_header(std::move(header))} -> std::same_as<header_validation_result>;
	};

	enum class session_state_status
	{
		completed,
		more_data_needed,
		client_error_detected,
		io_error
	};

	auto make_unique_cstr(char const* str)
	{
		auto n = strlen(str);
		auto ret = std::make_unique<char[]>(n + 1);
		memcpy(ret.get(), str, n);
		return ret;
	}

	struct session_state_response
	{
		session_state_status status;
		enum status http_status;
		std::unique_ptr<char[]> error_message;
	};

	class read_header_request
	{
	public:
		template<io::socket Socket, request_handler RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>& buffer,
			Socket const& socket,
			RequestHandler& req_handler);

	private:
		request_header_parser m_req_header_parser;
		req_header_parser_error_code m_saved_ec;
	};

	template<io::socket Socket, request_handler RequestHandler, size_t BufferSize>
	[[nodiscard]] auto read_header_request::operator()(
		io::buffer_view<char, BufferSize>& buffer,
		Socket const& socket,
		RequestHandler& req_handler)
	{
		while(true)
		{
			auto const parse_result = m_req_header_parser.parse(buffer.span_to_read());
			buffer.consume_until(parse_result.ptr);
			switch(parse_result.ec)
			{
				case req_header_parser_error_code::completed:
				{
					auto res = req_handler.set_header(m_req_header_parser.take_result());
					return session_state_response{
						.status = session_state_status::completed,
						.http_status = res.http_status,
						.error_message = std::move(res.error_message)
					};
				}

				case req_header_parser_error_code::more_data_needed:
				{
					auto const read_result = socket.read(buffer.span_to_write());
					buffer.reset_with_new_end(read_result.ptr);

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



#if 0
	template<io::socket Socket, request_handler RequestHandler, size_t BufferSize = 65536>
	class session
	{
	public:
		explicit session(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_connection{std::move(connection)},
			m_request_handler{std::move(req_handler)},
			m_current_state{state::read_request},
			m_buffer{std::make_unique<char[]>(BufferSize)},
			m_req_header{std::make_unique<request_header>()},
			m_req_header_parser{*m_req_header}
		{}

		void socket_is_ready();



	private:
		Socket m_connection;
		RequestHandler m_request_handler;
		enum class state{read_request_header, read_request_body, write_response};
		state m_current_state;

		// Use unique_ptr here to avoid creating self-referencing objects
		std::unique_ptr<char[]> m_buffer;
		std::span<char> m_remaining_read_span;

		// Use unique_ptr here to avoid creating self-referencing objects
		std::unique_ptr<request_header> m_req_header;

		// Use unique_ptr here to make prevent m_req_header_parser from outliving
		// m_req_header
		std::unique_ptr<request_header_parser> m_req_header_parser;

		void do_read_request_header();

		void do_read_request_body();
	};
}

template<west::io::socket Socket, west::http::request_handler RequestHandler, size_t BufferSize>
void west::http::session<Socket, RequestHandler, BufferSize>::socket_is_ready()
{
	switch(m_current_state)
	{
		case state::read_header_request:
			do_read_request_header();
			break;

		case state::read_request_body:
			do_read_request_body();
			break;

		case state::write_response:
			break;
	}
}

template<west::io::socket Socket, west::http::request_handler RequestHandler, size_t BufferSize>
void west::http::session<Socket, RequestHandler, BufferSize>::do_read_request_header()
{
	while(true)
	{
		auto const parse_result = m_req_header_parser->parse(m_remaining_read_span);
		switch(parse_result.ec)
		{
			case req_header_parser_error_code::completed:
				m_remaining_read_span = std::span{parse_result.ptr, std::end(m_remaining_read_span)};
				m_request_handler.set_header(std::move(*m_req_header));
				m_req_header_parser.reset();
				m_req_header.reset();
				m_current_state = state::read_request_body;
				break;

			case req_header_parser_error_code::more_data_needed:
			{
				auto const read_result = m_connection.read(std::span{m_buffer.get(), BufferSize});
				if(read_result.operation_would_block)
				{ return; }

				m_remaining_read_span = std::span{m_buffer.get(), read_result.ptr};
				break;
			}

			default:
				// TODO: handle parse error (as 4xy)
				break;
		}
	}
#endif
}
#endif