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
	template<class FdCallbackRegistry>
	class fd_callback_registry_ref
	{
	public:
		explicit fd_callback_registry_ref(FdCallbackRegistry& registry):
			m_registry{registry}
		{}
		
		template<class T>
		fd_callback_registry_ref& add(fd_ref fd, T&& l)		
		{
			m_registry.get().add(fd, std::forward<T>(l));
			return *this;
		}
		
		void remove(fd_ref fd)
		{ m_registry.get().deferred_remove(fd);}
		
	private:
		std::reference_wrapper<FdCallbackRegistry> m_registry;
	};

	class fd_event_monitor
	{
	public:		
		using listener_type = std::function<void(fd_ref, fd_callback_registry_ref<fd_event_monitor>)>;
		
		fd_event_monitor():m_fd{epoll_create1(0)}
		{
			if(m_fd == nullptr)
			{ throw system_error{"Failed to create epoll instance", errno}; }
		}

		[[nodiscard]] bool wait_for_and_dispatch_events()
		{
			if(std::size(m_listeners) == 0)
			{ return false; }

			std::span event_buffer{m_events.get(), std::size(m_listeners)};

			auto const n = ::epoll_wait(m_fd.get(),
				std::data(event_buffer),
				static_cast<int>(std::size(event_buffer)),
				-1);

			if(n == -1)
			{ throw system_error{"epoll_wait failed", errno}; }

			for(auto& event : std::span{m_events.get(), static_cast<size_t>(n)})
			{
				auto const data = static_cast<std::pair<fd_ref const, listener_type>*>(event.data.ptr);
				data->second(data->first, fd_callback_registry_ref{*this});
			}
			flush_fds_to_remove();
			return true;
		}

		template<class FdEventListener>
		fd_event_monitor& add(fd_ref fd, FdEventListener&& l)
		{
			assert(!m_listeners.contains(fd));

			auto const i = m_listeners.insert(std::pair{fd, std::forward<FdEventListener>(l)});
			epoll_event event{
				.events = EPOLLIN | EPOLLOUT,
				.data = &(*i.first)
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
		
		void defered_remove(fd_ref fd)
		{ m_fds_to_remove.push_back(fd); }
		
		void flush_fds_to_remove()
		{
			for(auto fd : m_fds_to_remove)
			{ 
				epoll_event event{};
				::epoll_ctl(m_fd.get(), EPOLL_CTL_DEL, fd , &event);
				m_listeners.erase(fd); 
			}

			m_fds_to_remove.clear();
		}

	private:
		fd_owner m_fd;

		std::unique_ptr<epoll_event[]> m_events;
		std::unordered_map<fd_ref, listener_type> m_listeners;
		std::vector<fd_ref> m_fds_to_remove;
	};
}

#endif
