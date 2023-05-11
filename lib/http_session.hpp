#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_request_header_parser.hpp"
#include "./http_response_header_serializer.hpp"
#include "./io_interfaces.hpp"

#include <span>
#include <memory>

namespace west::http
{
	struct header_validation_result
	{
	};


	template<class T>
	concept request_handler = requires(T x, request_header header)
	{
		{x.set_header(std::move(header))} -> std::same_as<header_validation_result>;
	};

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
}

#endif