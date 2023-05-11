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
			m_buffer_ptr{m_buffer.get()},
			m_req_header{std::make_unique<request_header>()},
			m_req_header_parser{*m_req_header}
		{}

		void socket_is_ready()
		{
			switch(m_current_state)
			{
				case state::read_request:
					break;
				case state::write_response:
					break;
			}
		}

	private:
		Socket m_connection;
		RequestHandler m_request_handler;
		enum class state{read_request, write_response};
		state m_current_state;

		std::unique_ptr<char[]> m_buffer;
		char* m_buffer_ptr;

		std::unique_ptr<request_header> m_req_header;
		std::unique_ptr<request_header_parser> m_req_header_parser;
	};
}

#endif