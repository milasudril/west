#ifndef WEST_SERVER_REGISTRY_HPP
#define WEST_SERVER_REGISTRY_HPP

#include "./io_fd.hpp"
#include "./io_fd_event_monitor.hpp"

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

	template<class T, class U, class... SessionArgs>
	concept session_factory = connection<U> && requires(T x, U&& y, SessionArgs... args)
	{
		{ x.create_session(std::forward<U>(y), args...) } -> session;
	};

	namespace detail
	{
		template<server_socket Socket>
		struct connection_type
		{
			using type = std::remove_cvref_t<decltype(std::declval<Socket>().accept())>;
		};
	}

	template<class T>
	struct session_state_mapper;

	template<class EventMonitor, server_socket ServerSocket, class SessionFactory, class... SessionArgs>
		requires session_factory<SessionFactory, typename detail::connection_type<ServerSocket>::type, SessionArgs...>
	void accept_connection(
		EventMonitor event_monitor,
		ServerSocket& server_socket,
		SessionFactory& session_factory,
		SessionArgs&&... session_args)
	{
		auto connection = server_socket.accept();
		connection.set_non_blocking();
		auto const conn_fd = connection.fd();
		event_monitor.add(conn_fd,
			[session = session_factory.create_session(std::move(connection), std::forward<SessionArgs>(session_args)...),
			 events = io::listen_on::read_is_possible]
			(auto event_monitor, io::fd_ref fd) mutable {
				auto result = session.socket_is_ready();
				if(is_session_terminated(result)) [[unlikely]]
				{
					event_monitor.remove(fd);
					return;
				}

				if(auto new_events = session_state_mapper<std::remove_cvref_t<decltype(result)>>{}(result); new_events != events)
				{
					event_monitor.modify(fd, new_events);
					events = new_events;
				}
			},
			io::listen_on::read_is_possible
		);
	}

	class service_registry
	{
	public:
		template<server_socket ServerSocket, class SessionFactory,	class... SessionArgs>
		requires session_factory<SessionFactory, typename detail::connection_type<ServerSocket>::type, SessionArgs...>
		service_registry& enroll(ServerSocket&& server_socket,
			SessionFactory&& session_factory,
			SessionArgs&&... session_args)
		{
			server_socket.set_non_blocking();
			auto const server_socket_fd = server_socket.fd();
			m_event_monitor.add(server_socket_fd,
				[server_socket = std::forward<ServerSocket>(server_socket),
					session_factory = std::forward<SessionFactory>(session_factory),
					... session_args = std::forward<SessionArgs>(session_args)
				](auto event_monitor, io::fd_ref) mutable {
					accept_connection(event_monitor, server_socket, session_factory, session_args...);
				});
			return *this;
		}

		service_registry& process_events()
		{
			while(m_event_monitor.wait_for_and_dispatch_events());
			return *this;
		}

		auto fd_callback_registry()
		{ return m_event_monitor.fd_callback_registry(); }

	private:
		io::fd_event_monitor m_event_monitor;
	};
}

#endif
