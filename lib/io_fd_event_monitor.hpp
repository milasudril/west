#ifndef WEST_IO_FD_EVENT_MONITOR_HPP
#define WEST_IO_FD_EVENT_MONITOR_HPP

#include "./system_error.hpp"
#include "./utils.hpp"
#include "./io_fd.hpp"

#include <sys/epoll.h>

namespace west::io
{
	enum class fd_event_result{remove_listener, keep_listener};

	template<class T>
	concept fd_event_listener = requires(T x)
	{
		{x()} -> std::same_as<fd_event_result>;
	};

	class fd_event_monitor
	{
	public:
		fd_event_monitor():m_fd{epoll_create1(0)}
		{
			if(m_fd == nullptr)
			{ throw system_error{"Failed to create epoll instance", errno}; }
		}

		template<fd_event_listener Listener>
		fd_event_monitor& add(struct fd fd, Listener&& l)
		{
			add_listener(fd, fd_listener{
				.closure = make_type_erased_ptr<Listener>(std::forward<Listener>(l)),
				.callback = [](void* object) {
					return (*static_cast<Listener*>(object))();
				},
				.fd = fd
			});
			return *this;
		}

	private:
		fd_owner m_fd;

		struct fd_listener
		{
			type_erased_ptr closure;
			fd_event_result (*callback)(void* object);
			struct fd fd;
		};

		void	add_listener(struct fd, fd_listener);
	};
}

#endif