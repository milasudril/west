//@	{"target":{"name":"io_inet_server_socket.test"}}

#include "./io_inet_server_socket.hpp"

#include <testfwk/testfwk.hpp>

#include <thread>

TESTCASE(west_io_inet_server_socket_bind_succesful)
{
	auto socket = west::io::create_socket(AF_INET, SOCK_STREAM, 0);

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
	auto socket = west::io::create_socket(AF_INET, SOCK_STREAM, 0);

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
			auto socket = connect_to(address, port);

			std::string_view msg_out{"Ping"};
			auto const n_written = ::write(socket.get(), std::data(msg_out), std::size(msg_out));
			EXPECT_EQ(static_cast<size_t>(n_written), std::size(msg_out));

			std::array<char, 4> msg_in{};
			auto const n_read = ::read(socket.get(), std::data(msg_in), std::size(msg_in));
			EXPECT_EQ(static_cast<size_t>(n_read), std::size(msg_in));
			EXPECT_EQ((std::string_view{std::data(msg_in), std::size(msg_in)}), "Pong");
		}
	};

	auto connection = server.accept();
	static_assert(west::io::socket<decltype(connection)>);
	EXPECT_EQ(connection.remote_address(), address);

	std::array<char, 4> msg_in{};
	auto const read_res = connection.read(msg_in);
	EXPECT_EQ(read_res.bytes_read, std::size(msg_in));
	EXPECT_EQ((std::string_view{std::data(msg_in), std::size(msg_in)}), "Ping");

	std::string_view msg_out{"Pong"};
	auto const write_res = connection.write(msg_out);
	EXPECT_EQ(write_res.bytes_written, std::size(msg_out));
}
