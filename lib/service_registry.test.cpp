//@	{"target":{"name":"service_registry.test"}}

#include "./service_registry.hpp"
#include "./io_inet_server_socket.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	enum class session_status{close_connection, keep_connection};
	
	constexpr bool is_session_terminated(session_status status)
	{ return status == session_status::close_connection; }
	
	struct session
	{
		west::io::inet_connection connection;
		session_status socket_is_ready()
		{ return session_status::keep_connection; }
	};
	
	struct factory
	{
		auto create_session(west::io::inet_connection&& connection)
		{
			return session{std::move(connection)};
		}
	};
}

TESTCASE(west_service_registry_enroll)
{
	west::service_registry registry{};
	west::io::inet_server_socket server_socket{
		west::io::inet_address{"127.0.0.1"},
		std::ranges::iota_view{49152, 65536},
		128
	};
	factory my_factory{};
	
	registry.enroll(std::move(server_socket), my_factory);
}
