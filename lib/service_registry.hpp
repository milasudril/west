#ifndef WEST_SERVER_REGISTRY_HPP
#define WEST_SERVER_REGISTRY_HPP

#include "./io_fd.hpp"
#include "./io_interfaces.hpp"
#include "./io_fd_event_monitor.hpp"

namespace west
{
	template<class T>
	concept connection = requires(T x)
	{
		{ x.fd() } -> std::same_as<io::fd_ref>;
		{ x.set_non_blocking() } -> std::same_as<void>;
	};	

	template<class T>
	concept server_socket = connection<T> && requires(T x)
	{
		{ x.accept() } -> connection;
	};
	
	template<class T>
	// HACK: An input fd is not a connection, but all members will have the same semantics
	concept input_fd = io::data_source<T> && connection<T>;

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

	template<class T>
	struct session_state_mapper;

	namespace detail
	{
		template<server_socket Socket>
		struct connection_type
		{
			using type = std::remove_cvref_t<decltype(std::declval<Socket>().accept())>;
		};
	}

	template<class Session>
	struct connection_event_handler
	{
		Session session;
		io::listen_on events;

		void fd_is_ready(auto event_monitor, io::fd_ref fd)
		{ finalize_event(session.socket_is_ready(), event_monitor, fd); }

		void fd_is_idle(auto event_monitor, io::fd_ref fd)
		{ finalize_event(session.socket_is_idle(), event_monitor, fd); }

		template<session_status SessionStatus, class EventMonitor>
		void finalize_event(SessionStatus&& status, EventMonitor event_monitor, io::fd_ref fd)
		{
			if(is_session_terminated(status))
			{
				event_monitor.remove(fd);
				return;
			}

			if(auto new_events = session_state_mapper<SessionStatus>{}(status);
				new_events != events)
			{
				event_monitor.modify(fd, new_events);
				events = new_events;
			}
		}
	};


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
			connection_event_handler{
				session_factory.create_session(std::move(connection), std::forward<SessionArgs>(session_args)...),
				io::listen_on::read_is_possible
			},
			io::listen_on::read_is_possible
		);
	}

	template<server_socket ServerSocket, class SessionFactory, class... SessionArgs>
	struct server_event_handler
	{
		ServerSocket server_socket;
		SessionFactory session_factory;
		std::tuple<SessionArgs...>  session_args;

		void fd_is_ready(auto event_monitor, io::fd_ref)
		{
			std::apply([this, event_monitor](auto... session_args){
				accept_connection(event_monitor, server_socket, session_factory, session_args...);
			}, session_args);
		}

		void fd_is_idle(auto, io::fd_ref)
		{}
	};
	
	template<class InputFd, class InputFdEventHandler>
	struct data_source_event_handler
	{
		void fd_is_ready(auto event_monitor, io::fd_ref fd)
		{ eh.fd_is_ready(event_monitor, fd); }
		
		void fd_is_idle(auto event_monitor, io::fd_ref fd)
		{ eh.fd_is_idle(event_monitor, fd); }		
		
		InputFd fd;
		InputFdEventHandler eh;
	};

	class service_registry
	{
	public:
		static constexpr auto inactivity_period = io::fd_event_monitor::inactivity_period;

		template<server_socket ServerSocket, class SessionFactory,	class... SessionArgs>
		requires session_factory<SessionFactory, typename detail::connection_type<ServerSocket>::type, SessionArgs...>
		service_registry& enroll(ServerSocket&& server_socket,
			SessionFactory&& session_factory,
			SessionArgs&&... session_args)
		{
			server_socket.set_non_blocking();
			auto const server_socket_fd = server_socket.fd();
			m_event_monitor.add(server_socket_fd,
				server_event_handler{
					std::forward<ServerSocket>(server_socket),
					std::forward<SessionFactory>(session_factory),
					std::tuple{std::forward<SessionArgs>(session_args)...}
				}
			);
			return *this;
		}

		template<input_fd InputFd, class InputFdEventHandler>
		service_registry& enroll(InputFd&& data_source, InputFdEventHandler&& eh)
		{
			data_source.set_non_blocking();
			auto const data_source_fd = data_source.fd();
			m_event_monitor.add(data_source_fd, 
				data_source_event_handler{std::forward<InputFd>(data_source),
					std::forward<InputFdEventHandler>(eh)},
				io::listen_on::read_is_possible);
			
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
