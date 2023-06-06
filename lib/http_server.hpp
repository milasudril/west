#ifndef WEST_HTTP_SERVER_HPP
#define WEST_HTTP_SERVER_HPP

#include "./io_fd_event_monitor.hpp"
#include "./http_request_processor.hpp"

template<>
struct west::session_state_mapper<west::http::process_request_result>
{
	constexpr auto operator()(http::process_request_result res) const
	{ 
		switch(res.io_dir)
		{
			case http::session_state_io_direction::input:
				return io::listen_on::read_is_possible;
			case http::session_state_io_direction::output:
				return io::listen_on::write_is_possible;
			default:
				__builtin_unreachable();
		}
	}
};

#endif
