#ifndef WEST_IO_FD_EVENT_MONITOR_HPP
#define WEST_IO_FD_EVENT_MONITOR_HPP

#include "./system_error.hpp"
#include "./io_fd.hpp"

#include <sys/epoll.h>

#include <vector>
#include <unordered_map>
#include <span>
#include <cassert>

namespace west::io
{
	template<class T>
	concept fd_event_listener = requires(T x)
	{
		{x()} -> std::same_as<void>;
	};

	template<fd_event_listener FdEventListener>
	class fd_event_monitor
	{
	public:
		fd_event_monitor():m_fd{epoll_create1(0)}
		{
			if(m_fd == nullptr)
			{ throw system_error{"Failed to create epoll instance", errno}; }
		}

		void wait_for_events()
		{
			if(std::size(m_listeners) == 0)
			{ return; }

			std::span event_buffer{m_events.get(), std::size(m_listeners)};

			auto const n = ::epoll_wait(m_fd.get(),
				std::data(event_buffer),
				static_cast<int>(std::size(event_buffer)),
				-1);

			if(n == -1)
			{ throw system_error{"epoll_wait failed", errno}; }

			for(auto& event : std::span{m_events.get(), static_cast<size_t>(n)})
			{
				auto const data = static_cast<event_data*>(event.data.ptr);
				data->listener();
			}
		}

		fd_event_monitor& remove(struct fd fd)
		{
			::epoll_ctl(m_fd.get(), EPOLL_CTL_DEL, fd, nullptr);
			m_listeners.erase(fd);
			return *this;
		}

		fd_event_monitor& add(struct fd fd, FdEventListener&& l)
		{
			assert(!m_listeners.contains(fd));

			auto const i = m_listeners.insert(std::pair{fd, event_data{std::move(l), fd}});
			epoll_event event{
				.events = EPOLLIN | EPOLLOUT,
				.data = &i.first->second
			};

			if(::epoll_ctl(m_fd.get(), EPOLL_CTL_ADD, fd, &event) == -1)
			{
				auto const saved_errno = errno;
				m_listeners.erase(i.second);
				throw system_error{"Failed to add event listener for file descriptor", saved_errno};
			}

			m_events = std::make_unique_for_overwrite<epoll_event[]>(std::size(m_listeners));
			return *this;
		}

	private:
		fd_owner m_fd;
		struct event_data
		{
			FdEventListener listener;
			struct fd const fd;
		};
		std::unique_ptr<epoll_event[]> m_events;
		std::unordered_map<fd, event_data> m_listeners;
	};
}

#endif