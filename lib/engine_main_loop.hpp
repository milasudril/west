#ifndef WEST_ENGINE_MAINLOOP_HPP
#define WEST_ENGINE_MAINLOOP_HPP

#include "./io_fd.hpp"
#include "./io_fd_event_monitor.hpp"

#include <algorithm>

namespace west::engine
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
	void main_loop(ServerSocket&& server_socket, SessionFactory&& session_factory)
	{
		server_socket.set_non_blocking();
		io::fd_event_monitor<std::function<void()>> monitor{};
		auto const server_socket_fd = server_socket.fd();
		std::vector<io::fd_ref> connections_to_remove;
		monitor.add(server_socket_fd,
			[server_socket = std::forward<ServerSocket>(server_socket),
				&monitor,
				&connections_to_remove,
				session_factory = std::forward<SessionFactory>(session_factory)
			](){
				auto connection = server_socket.accept();
				auto const conn_fd = connection.fd();
				monitor.add(conn_fd, [session = session_factory.create(std::move(connection)),
					conn_fd,
					&connections_to_remove](){
					if(is_session_terminated(session.socket_is_ready()))
					{ connections_to_remove.push_back(conn_fd); }
				});
		});

		while(true)
		{
			monitor.wait_for_events();
			std::ranges::for_each(connections_to_remove, [&monitor](auto item){
				monitor.remove(item);
			});
			connections_to_remove.clear();
		}
	}
}

#endif