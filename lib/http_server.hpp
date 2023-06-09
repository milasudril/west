#ifndef WEST_HTTP_SERVER_HPP
#define WEST_HTTP_SERVER_HPP

#include "./service_registry.hpp"
#include "./http_request_processor.hpp"
#include "./http_session_factory.hpp"
#include "./http_request_handler.hpp"

template<>
struct west::session_state_mapper<west::http::process_request_result>
{
	constexpr auto operator()(http::process_request_result res) const
	{
		switch(res.io_dir)
		{
			case http::session_state_io_direction::input:
				return io::listen_on::read_is_possible;
			case http::session_state_io_direction::output:
				return io::listen_on::write_is_possible;
			default:
				__builtin_unreachable();
		}
	}
};

namespace west
{
	template<http::request_handler RequestHandler, server_socket ServerSocket, class... SessionArgs>
	auto& enroll_http_service(service_registry& registry,
		ServerSocket&& server,
		SessionArgs&&... session_args)
	{
		return registry.enroll(std::forward<ServerSocket>(server),
			http::session_factory<RequestHandler>{},
			std::forward<SessionArgs>(session_args)...);
	}
}

#endif
