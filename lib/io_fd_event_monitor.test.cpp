//@	{"target":{"name":"io_fd_event_monitor.test"}}

#include "./io_fd_event_monitor.hpp"

#include <testfwk/testfwk.hpp>

#include <thread>
#include <chrono>

namespace
{
	struct callback
	{
		std::atomic<int> ready_callcount{0};
		std::atomic<int> idle_callcount{0};

		template<class... T>
		void fd_is_ready(T&&...)
		{ ++ready_callcount; }

		template<class... T>
		void fd_is_idle(T&&...)
		{ ++idle_callcount; }
	};

	template<class Duration, class Callable, class ... Args>
	[[nodiscard]] auto expectBlockForAtLeast(Duration d, Callable&& f, Args&&... args)
	{
		return std::jthread{[d, func = std::forward<Callable>(f)]<class ... T>(auto&&, T&&... args) {
			auto const t0 = std::chrono::steady_clock::now();
			func(std::forward<T>(args)...);
			auto const t1 = std::chrono::steady_clock::now();
			EXPECT_GE(t1 - t0, d);
		}, std::forward<Args>(args)...};
	}
}

TESTCASE(west_io_fd_event_monitor_monitor_pipe_read_end)
{
	west::io::fd_event_monitor monitor{};
	auto pipe = west::io::create_pipe(O_NONBLOCK | O_DIRECT);

	callback read_activated{};

	// No listener. wait_for_and_dispatch_events would return immediately
	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), false);
	EXPECT_EQ(read_activated.ready_callcount, 0);
	EXPECT_EQ(read_activated.idle_callcount, 0);

	monitor.add(pipe.read_end.get(), west::io::fd_event_listener_ref{read_activated});


	// Wait for, and write some data
	{
		auto _ = expectBlockForAtLeast(std::chrono::milliseconds{125}, [&monitor]() {
			EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
		});

		std::this_thread::sleep_for(std::chrono::milliseconds{250});

		{
			std::string_view buffer{"Hello, World"};
			auto res = ::write(pipe.write_end.get(), std::data(buffer), std::size(buffer));
			EXPECT_EQ(res, std::ssize(buffer));
		}
	}
	EXPECT_EQ(read_activated.ready_callcount, 1);
	EXPECT_EQ(read_activated.idle_callcount, 0);

	// Listener is still active since there is more data to read
	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
	EXPECT_EQ(read_activated.ready_callcount, 2);
	EXPECT_EQ(read_activated.idle_callcount, 0);

	// Draining the pipe should cause wait_for_data to block
	while(true)
	{
		std::array<char, 12> buffer{};
		auto const res = ::read(pipe.read_end.get(), std::data(buffer), std::size(buffer));
		if(res == -1)
		{
			EXPECT_EQ(errno == EWOULDBLOCK || errno == EAGAIN, true);
			break;
		}
		else
		if(res == 0)
		{ break; }
		else
		{ EXPECT_EQ(res, std::ssize(buffer)); }
	}

	// Wait for, and write some data again
	{
		auto _ = expectBlockForAtLeast(std::chrono::milliseconds{125}, [&monitor]() {
			EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
		});

		std::this_thread::sleep_for(std::chrono::milliseconds{250});

		{
			std::string_view buffer{"Hello, World"};
			auto res = ::write(pipe.write_end.get(), std::data(buffer), std::size(buffer));
			EXPECT_EQ(res, std::ssize(buffer));
		}
	}
	EXPECT_EQ(read_activated.ready_callcount, 3);
}

TESTCASE(west_io_fd_event_monitor_monitor_pipe_write_end)
{
	west::io::fd_event_monitor monitor{};
	auto pipe = west::io::create_pipe(O_NONBLOCK | O_DIRECT);

	callback write_activated{};

	// No listener. wait_for_and_dispatch_events would return immediately
	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), false);
	EXPECT_EQ(write_activated.ready_callcount, 0);
	EXPECT_EQ(write_activated.idle_callcount, 0);

	monitor.add(pipe.write_end.get(), west::io::fd_event_listener_ref{write_activated});

	// Write should be possible now. It is woken, because write is initially possible.
	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
	EXPECT_EQ(write_activated.ready_callcount, 1);
	EXPECT_EQ(write_activated.idle_callcount, 0);

	// Fill the pipe
	while(true)
	{
		std::array<char, 65536> buffer{};
		auto res = ::write(pipe.write_end.get(), std::data(buffer), std::size(buffer));
		if(res == -1)
		{
			EXPECT_EQ(errno == EWOULDBLOCK || errno == EAGAIN, true);
			break;
		}
	}

	// Read from the pipe to see that the write end becomes available again
	{
		auto _ = expectBlockForAtLeast(std::chrono::milliseconds{125}, [&monitor]() {
			EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
		});

		std::this_thread::sleep_for(std::chrono::milliseconds{250});

		{
			std::array<char, 134> buffer{};
			auto res = ::read(pipe.read_end.get(), std::data(buffer), std::size(buffer));
			EXPECT_EQ(res, std::ssize(buffer));
		}
	}

	EXPECT_EQ(write_activated.ready_callcount, 2);
	EXPECT_EQ(write_activated.idle_callcount, 0);
}

TESTCASE(west_io_fd_event_monitor_idle_fd)
{
	west::io::fd_event_monitor monitor{};
	callback cb{};

	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), false);
	EXPECT_EQ(cb.idle_callcount, 0);
	EXPECT_EQ(cb.ready_callcount, 0);

	auto pipe = west::io::create_pipe(O_NONBLOCK | O_DIRECT);

	monitor.add(pipe.read_end.get(), west::io::fd_event_listener_ref{cb}, west::io::listen_on::read_is_possible);

	auto const t0 = std::chrono::steady_clock::now();
	while(std::chrono::steady_clock::now() - t0 < west::io::fd_event_monitor::inactivity_period)
	{
		EXPECT_EQ(cb.idle_callcount, 0);
		EXPECT_EQ(cb.ready_callcount, 0);
		EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
	}
	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
	EXPECT_EQ(monitor.wait_for_and_dispatch_events(), true);
	EXPECT_EQ(cb.idle_callcount, 1);
	EXPECT_EQ(cb.ready_callcount, 0);
}