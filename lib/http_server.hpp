#ifndef WEST_HTTP_SERVER_HPP
#define WEST_HTTP_SERVER_HPP

#include "./service_registry.hpp"
#include "./http_request_processor.hpp"
#include "./http_session_factory.hpp"

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
	template<class RequestHandler, class... SessionArgs>
	auto& enroll_http_service(service_registry& registry,
		io::inet_server_socket&& server,
		io::inet_connection_opts&& connection_opts,
		SessionArgs&&... session_args)
	{
		return registry.enroll(std::move(server),
			std::move(connection_opts),
			http::session_factory<RequestHandler>{},
			std::forward<SessionArgs>(session_args)...);
	}

	template<class RequestHandler, class... SessionArgs>
	auto& enroll_http_service(service_registry& registry,
		io::inet_server_socket&& server,
		SessionArgs&&... session_args)
	{
		return enroll_http_service<RequestHandler>(registry,
			std::move(server),
			io::inet_connection_opts{},
			std::forward<SessionArgs>(session_args)...);
	}
}

#endif
