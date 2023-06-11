#ifndef WEST_IO_FD_EVENT_MONITOR_HPP
#define WEST_IO_FD_EVENT_MONITOR_HPP

#include "./system_error.hpp"
#include "./io_fd.hpp"
#include "./utils.hpp"

#include <sys/epoll.h>

#include <vector>
#include <unordered_map>
#include <span>
#include <cassert>
#include <chrono>
#include <list>

namespace west::io
{
	enum class listen_on {
		read_is_possible = EPOLLIN,
		write_is_possible = EPOLLOUT,
		readwrite_is_possible = EPOLLIN|EPOLLOUT
	};

	template<class FdCallbackRegistry>
	class fd_callback_registry_ref
	{
	public:
		explicit fd_callback_registry_ref(FdCallbackRegistry& registry):
			m_registry{registry}
		{}

		template<class T>
		fd_callback_registry_ref& add(fd_ref fd, T&& l, listen_on events = listen_on::readwrite_is_possible)
		{
			m_registry.get().add(fd, std::forward<T>(l), events);
			return *this;
		}

		void modify(fd_ref fd, listen_on new_events)
		{ m_registry.get().modify(fd, new_events); }

		void remove(fd_ref fd)
		{ m_registry.get().deferred_remove(fd);}

		void clear()
		{ m_registry.get().deferred_clear(); }

	private:
		std::reference_wrapper<FdCallbackRegistry> m_registry;
	};

	class fd_event_monitor
	{
	public:
		using activity_timestamp = std::chrono::steady_clock::time_point;
		using fd_activity_list = std::list<std::pair<activity_timestamp, fd_ref>>;

		static constexpr auto inactivity_period = std::chrono::seconds{10};

		class listener
		{
		public:
			template<class FdEventListener>
			explicit listener(FdEventListener&& object, fd_activity_list::iterator const& timer):
				m_object{make_type_erased_ptr<FdEventListener>(std::forward<FdEventListener>(object))},
				m_callback{[](void* obj, fd_callback_registry_ref<fd_event_monitor> registry, fd_ref fd) {
					auto& l = *static_cast<FdEventListener*>(obj);
					l(registry, fd);
				}},
				m_timer{timer}
			{}

			void invoke(fd_callback_registry_ref<fd_event_monitor> callback_registry, fd_ref fd, fd_activity_list&)
			{
				fprintf(stderr, "Activity on fd %d\n", static_cast<int>(fd));
				m_callback(m_object.get(), callback_registry, fd);

			//	assert(m_timer.has_value());
			//	auto const now = std::chrono::steady_clock::now();
			//	(*m_timer)->first = now + inactivity_period;
			//	list.splice(list.end(), list, *m_timer);
			}

			void remove_from(fd_activity_list&)
			{
			//	fprintf(stderr, "Removing entry from list %p\n", this);
			//	assert(m_timer.has_value());
			//	auto const timer = *m_timer;
			//	m_timer.reset();
			//	list.erase(m_timer);
			}

		private:
			type_erased_ptr m_object;
			void (*m_callback)(void*, fd_callback_registry_ref<fd_event_monitor>, fd_ref);
		//	std::optional<fd_activity_list::iterator> m_timer;
			fd_activity_list::iterator m_timer;
		};

		fd_event_monitor():
			m_fd{epoll_create1(0)},
			m_event_buffer_capacity{0},
			m_reg_should_be_cleared{false}
		{
			if(m_fd == nullptr)
			{ throw system_error{"Failed to create epoll instance", errno}; }
		}

		auto fd_callback_registry()
		{ return fd_callback_registry_ref{*this}; }

		[[nodiscard]] bool wait_for_and_dispatch_events()
		{
			auto const num_listeners = std::size(m_listeners);

			if(num_listeners == 0)
			{ return false; }

			if(num_listeners > m_event_buffer_capacity)
			{
				m_events = std::make_unique_for_overwrite<epoll_event[]>(num_listeners);
				m_event_buffer_capacity = num_listeners;
			}

			std::span event_buffer{m_events.get(), num_listeners};

			auto const n = ::epoll_wait(m_fd.get(),
				std::data(event_buffer),
				static_cast<int>(std::size(event_buffer)),
				-1);

			if(n == -1)
			{ throw system_error{"epoll_wait failed", errno}; }

			for(auto& event : std::span{m_events.get(), static_cast<size_t>(n)})
			{
				auto const data = static_cast<std::pair<fd_ref const, listener>*>(event.data.ptr);
				data->second.invoke(fd_callback_registry(), data->first, m_fd_activity_timestamps);
			}
			flush_fds_to_remove();
			return true;
		}

		template<class FdEventListener>
		fd_event_monitor& add(fd_ref fd, FdEventListener&& l, listen_on events = listen_on::readwrite_is_possible)
		{
			assert(!m_listeners.contains(fd));

			auto const now = std::chrono::steady_clock::now();
			m_fd_activity_timestamps.push_back(std::pair{now + inactivity_period, fd});
			auto const timer = m_fd_activity_timestamps.rbegin().base();

			auto const i = m_listeners.insert(std::pair{
				fd,
				listener{std::forward<FdEventListener>(l), timer}
			});

			epoll_event event{
				.events = static_cast<uint32_t>(events),
				.data = &(*i.first)
			};

			if(::epoll_ctl(m_fd.get(), EPOLL_CTL_ADD, fd, &event) == -1)
			{
				auto const saved_errno = errno;
				m_listeners.erase(i.first);
				throw system_error{"Failed to add event listener for file descriptor", saved_errno};
			}

			return *this;
		}

		void modify(fd_ref fd, listen_on new_events)
		{
			auto const i = m_listeners.find(fd);
			assert(i != std::end(m_listeners));
			epoll_event event{
				.events = static_cast<uint32_t>(new_events),
				.data = &(*i)
			};
			if(::epoll_ctl(m_fd.get(), EPOLL_CTL_MOD, fd, &event) == -1)
			{ throw system_error{"Failed to modify event listener", errno}; }
		}

		void deferred_remove(fd_ref fd)
		{ m_fds_to_remove.push_back(fd); }

		void deferred_clear()
		{ m_reg_should_be_cleared = true; }

		void flush_fds_to_remove()
		{
			if(m_reg_should_be_cleared)
			{ *this = fd_event_monitor{}; }
			else
			{
				for(auto fd : m_fds_to_remove)
				{
					epoll_event event{};
					::epoll_ctl(m_fd.get(), EPOLL_CTL_DEL, fd , &event);

					auto const i = m_listeners.find(fd);
					assert(i != std::end(m_listeners));
					assert(!m_fd_activity_timestamps.empty());
					assert(fd != fd_ref{0});
					i->second.remove_from(m_fd_activity_timestamps);
					m_listeners.erase(i);
				}

				m_fds_to_remove.clear();
			}
		}

	private:
		fd_owner m_fd;

		std::unique_ptr<epoll_event[]> m_events;
		size_t m_event_buffer_capacity;
		std::unordered_map<fd_ref, listener> m_listeners;
		std::vector<fd_ref> m_fds_to_remove;
		fd_activity_list m_fd_activity_timestamps;
		bool m_reg_should_be_cleared;
	};
}

#endif
