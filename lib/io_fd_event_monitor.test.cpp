//@	{"target":{"name":"io_fd_event_monitor.test"}}

#include "./io_fd_event_monitor.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_io_fd_event_monitor_create)
{
	west::io::fd_event_monitor monitor{};
}
