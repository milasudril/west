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
	template<class RequestHandler, class ... Args>
	auto& enroll_http_service(service_registry& registry,
		io::inet_server_socket&& server,
		Args&& ... session_factory_args)
	{
		return registry.enroll(std::move(server),
			http::session_factory<RequestHandler>{std::forward<Args>(session_factory_args)...});
	}
}

#endif
