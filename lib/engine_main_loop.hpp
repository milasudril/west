#ifndef WEST_ENGINE_MAINLOOP_HPP
#define WEST_ENGINE_MAINLOOP_HPP

#include "./io_fd.hpp"
#include "./io_fd_event_monitor.hpp"

#include <algorithm>

namespace west
{
	template<class T>
	concept connection = requires(T x)
	{
		{ x.fd() } -> std::same_as<io::fd_ref>;
	};

	template<class T>
	concept server_socket = requires(T x)
	{
		{ x.accept() } -> connection;
		{ x.fd() } -> std::same_as<io::fd_ref>;
	};

	template<class T>
	concept session_status = requires(T x)
	{
		{ is_session_terminated(x) } -> std::same_as<bool>;
	};

	template<class T>
	concept session = requires(T x)
	{
		{ x.socket_is_ready() } -> session_status;
	};

	template<class T, class U>
	concept session_factory = connection<U> && requires(T x, U&& y)
	{
		{ x.create_session(std::forward<U>(y)) } -> session;
	};

	namespace detail
	{
		template<server_socket Socket>
		struct connection_type
		{
			using type = std::remove_cvref_t<decltype(std::declval<Socket>().accept())>;
		};
	}
	
	template<server_socket ServerSocket,
		session_factory<typename detail::connection_type<ServerSocket>::type> SessionFactory>
	void accept_connection(
		io::fd_event_monitor& event_monitor,
		ServerSocket& server_socket, 
		SessionFactory& session_factory)
	{
		auto connection = server_socket.accept();
		connection.set_non_blocking();
		auto const conn_fd = connection.fd();
		event_monitor.add(conn_fd,
			[session = session_factory.create(std::move(connection))]
			(auto event_monitor, io::fd_ref fd){
				if(is_session_terminated(session.socket_is_ready()))
				{ event_monitor.remove(fd); }
			}
		);
	}
	
	class server_registry
	{
	public:
		template<server_socket ServerSocket,
			session_factory<typename detail::connection_type<ServerSocket>::type> SessionFactory>
		server_registry& enroll(io::fd_event_monitor& event_monitor,
			ServerSocket&& server_socket,
			SessionFactory&& session_factory)
		{
			server_socket.set_non_blocking();
			auto const server_socket_fd = server_socket.fd();
			m_event_monitor.add(server_socket_fd,
				[server_socket = std::forward<ServerSocket>(server_socket),
					session_factory = std::forward<SessionFactory>(session_factory)
				](auto event_monitor, io::fd_ref){
					accept_connection(event_monitor, server_socket, session_factory);
				});
			return *this;
		}
		
		server_registry& run_main_loop()
		{
			while(m_event_monitor.wait_for_and_dispatch_events());
			return *this;
		}
	
	private:
		io::fd_event_monitor m_event_monitor;
	};
}

#endif
