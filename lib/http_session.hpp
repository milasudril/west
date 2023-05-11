#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_request_header_parser.hpp"
#include "./http_response_header_serializer.hpp"

#include <span>

namespace west
{
	struct read_result
	{
		char* ptr;
		bool would_block;
	};

	struct write_result
	{
		char const* ptr;
		bool would_block;
	};

	struct header_validation_result
	{
	};

	template<class T>
	concept socket = requires(T x, std::span<char> y, std::span<char const> z)
	{
		{x.read(y)} -> std::same_as<read_result>;
		{x.write(z)} -> std::same_as<write_result>;
		{x.stop_reading()} -> std::same_as<void>;
	};

	template<class T>
	concept http_request_handler = requires(T x, http::request_header header)
	{
		{x.set_header(std::move(header))} -> std::same_as<header_validation_result>;
	};

	template<socket Socket, http_request_handler RequestHandler>
	class http_session
	{
	public:
		explicit http_session(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_connection{std::move(connection)},
			m_request_handler{std::move(req_handler)},
			m_current_state{state::read_request},
			m_header_parser{m_req_header}
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

		// FIXME: Should not have self-referencial classes
		http::request_header m_req_header;
		http::request_header_parser m_header_parser;
	};
}

#endif