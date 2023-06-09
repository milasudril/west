#ifndef WEST_HTTP_SESSION_FACTORY_HPP
#define WEST_HTTP_SESSION_FACTORY_HPP

#include "./http_request_processor.hpp"

namespace west::http
{
	template<request_handler RequestHandler>
	struct session_factory
	{
		template<io::socket Socket, class... SessionArgs>
		auto create_session(Socket&& socket, SessionArgs&&... session_args)
		{
			return request_processor{
				std::forward<Socket>(socket),
				RequestHandler{std::forward<SessionArgs>(session_args)...}
			};
		}
	};
}

#endif
