//@	{"target":{"name":"io_inet_server_socket.test"}}

#include "./io_inet_server_socket.hpp"

#include <testfwk/testfwk.hpp>

#include <thread>

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

TESTCASE(west_io_inet_server_socket_accept_connection)
{
	west::io::inet_address address{"127.0.0.1"};

	west::io::inet_server_socket server{
		address,
		std::ranges::iota_view{49152, 65536},
		128
	};

	std::jthread client{
		[port = server.port(), address](){
			auto socket = west::io::socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr = address.value();

			auto const res = connect(socket.get(),
				reinterpret_cast<sockaddr const*>(&addr),
				sizeof(addr));

			REQUIRE_NE(res, -1);
/*
			std::array<char, 12> buffer{};
			auto const n = ::read(socket, std::data(buffer), std::size(buffer));
			EXPECT_EQ(n, std::size(buffer));
			EXPECT_EQ(std::string_view{std::data(buffer), 12}, "Hello, World");
*/
		}
	};

	auto const connection = server.accept();
	EXPECT_EQ(connection.remote_address(), address);
}