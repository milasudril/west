//@	{"target":{"name":"io_inet_server_socket.test"}}

#include "./io_inet_server_socket.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_io_inet_server_socket_bind_succesful)
{
	auto socket = west::io::socket(AF_INET, SOCK_STREAM, 0);

	EXPECT_NE(socket, nullptr);
	auto port = bind(socket.get(),
		west::io::inet_address{"127.0.0.1"},
		std::ranges::iota_view{49152, 65536},
		[](auto, auto, int port) {
			return port == 49762 ? 0 : -1;
	});
	EXPECT_EQ(port, 49762);
}

TESTCASE(west_io_inet_server_socket_bind_failed)
{
	auto socket = west::io::socket(AF_INET, SOCK_STREAM, 0);

	EXPECT_NE(socket, nullptr);

	try
	{
		(void)bind(socket.get(),
			west::io::inet_address{"127.0.0.1"},
			std::ranges::iota_view{49152, 65536},
			[](auto...) {
				errno = EADDRINUSE;
				return -1;
		});
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Failed to bind socket: Address already in use"});
	}
}