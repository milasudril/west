//@	{"target":{"name":"io_fd_event_monitor.test"}}

#include "./io_fd_event_monitor.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct callback
	{
		 west::io::fd_event_result operator()() const
		 {
			 return west::io::fd_event_result::keep_listener;
		 }
	};
}

TESTCASE(west_io_fd_event_monitor_create)
{
	west::io::fd_event_monitor<callback> monitor{};
}