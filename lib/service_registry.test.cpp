//@	{"target":{"name":"service_registry.test"}}

#include "./service_registry.hpp"
#include "./io_inet_server_socket.hpp"

#include <testfwk/testfwk.hpp>
#include <thread>
#include <algorithm>
#include <random>

namespace
{
	enum class session_status{close_connection, keep_connection};

	constexpr bool is_session_terminated(session_status status)
	{ return status == session_status::close_connection; }

	struct session
	{
		west::io::inet_connection connection;
		west::io::fd_ref server_fd;
		west::io::fd_callback_registry_ref<west::io::fd_event_monitor> event_monitor;
		session_status socket_is_ready()
		{
			std::array<char, 65536> buffer{};
			while(true)
			{
				auto res = connection.read(buffer);
				if(res.bytes_read != 0)
				{
					std::string_view const str{std::data(buffer), res.bytes_read};
					if(str == "shutdown")
					{ event_monitor.remove(server_fd); }
					else
					{
						EXPECT_EQ((std::string_view{std::data(buffer), res.bytes_read}), "give me some data");
						(void)connection.write(std::string_view{"here are some data"});
					}
				}

				if(res.bytes_read == 0)
				{
					switch(res.ec)
					{
						case west::io::operation_result::operation_would_block:
							return session_status::keep_connection;
						case west::io::operation_result::completed:
							return session_status::close_connection;
						case west::io::operation_result::error:
							return session_status::close_connection;
					}
				}
			}
		}

		session_status socket_is_idle()
		{ return session_status::close_connection; }
	};

	struct factory
	{
		west::io::fd_ref server_fd;
		west::io::fd_callback_registry_ref<west::io::fd_event_monitor> event_monitor;

		auto create_session(west::io::inet_connection&& connection)
		{
			return session{std::move(connection), server_fd, event_monitor};
		}
	};
}

template<>
struct west::session_state_mapper<session_status>
{
	template<class T>
	constexpr auto operator()(T) const
	{ return io::listen_on::readwrite_is_possible; }
};

TESTCASE(west_service_registry_enroll)
{
	west::io::inet_address address{"127.0.0.1"};
	west::io::inet_server_socket server_socket{
		address,
		std::ranges::iota_view{49152, 65536},
		128
	};

	auto const server_port = server_socket.port();
	std::jthread server_thread{[server_socket = std::move(server_socket)]() mutable {
		west::service_registry registry{};

		registry
			.enroll(std::move(server_socket), factory{server_socket.fd(), registry.fd_callback_registry()})
			.process_events();
	}};

	enum class client_action{connect, write_request, read_response, disconnect};
	struct client_state
	{
		client_action next_action{client_action::connect};
		west::io::fd_owner socket;
	};

	std::array<client_state, 16> clients{};

	auto all_clients_disconnected = [&clients](){
		return std::ranges::all_of(clients, [](auto const& item) {
			return item.next_action == client_action::disconnect && item.socket == nullptr;
		});
	};

	std::uniform_int_distribution U{static_cast<size_t>(0), std::size(clients) - 1};
	std::random_device rng;
	while(!all_clients_disconnected())
	{
		auto const client_index = U(rng);
		auto& client = clients[client_index];
		if(client.next_action == client_action::disconnect && client.socket == nullptr)
		{ continue; }

		switch(client.next_action)
		{
			case client_action::connect:
				client.socket = connect_to(address, server_port);
				client.next_action = client_action::write_request;
				break;

			case client_action::write_request:
			{
				std::string_view buffer{"give me some data"};
				EXPECT_EQ(::write(client.socket.get(), std::data(buffer), std::size(buffer)), std::ssize(buffer));
				client.next_action = client_action::read_response;
				break;
			}

			case client_action::read_response:
			{
				std::string_view expected_result{"here are some data"};
				std::array<char, 65536> buffer{};
				EXPECT_EQ(::read(client.socket.get(), std::data(buffer), std::size(buffer)), std::ssize(expected_result));
				EXPECT_EQ((std::string_view{std::data(buffer), std::size(expected_result)}), expected_result);
				client.next_action = client_action::disconnect;
				break;
			}

			case client_action::disconnect:
				client.socket.reset();
				break;
		}
	}

	{
		auto socket = connect_to(address, server_port);
		std::string_view buffer{"shutdown"};
		EXPECT_EQ(::write(socket.get(), std::data(buffer), std::size(buffer)), std::ssize(buffer));
	}
}

TESTCASE(west_service_registry_socket_is_idle)
{
	west::io::inet_address address{"127.0.0.1"};
	west::io::inet_server_socket server_socket{
		address,
		std::ranges::iota_view{49152, 65536},
		128
	};

	auto const server_port = server_socket.port();
	std::jthread server_thread{[server_socket = std::move(server_socket)]() mutable {
		west::service_registry registry{};

		registry
			.enroll(std::move(server_socket), factory{server_socket.fd(), registry.fd_callback_registry()})
			.process_events();
	}};

	auto const t0 = std::chrono::steady_clock::now();
	auto connection = connect_to(address, server_port);
	{
		auto socket = connect_to(address, server_port);
		std::string_view buffer{"shutdown"};
		EXPECT_EQ(::write(socket.get(), std::data(buffer), std::size(buffer)), std::ssize(buffer));
	}
	server_thread.join();
	EXPECT_GE(std::chrono::steady_clock::now() - t0, west::service_registry::inactivity_period);
}
