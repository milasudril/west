#ifndef WEST_IO_INET_SERVER_SOCKET_HPP
#define WEST_IO_INET_SERVER_SOCKET_HPP

#include "./io_fd.hpp"
#include "./system_error.hpp"
#include "./io_interfaces.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstring>
#include <ranges>
#include <stdexcept>

namespace west::io
{
	class inet_address
	{
	public:
		explicit inet_address(in_addr value): m_value{value}{}

		explicit inet_address(char const* str)
		{
			if(inet_aton(str, &m_value) == 0)
			{ throw std::runtime_error{"Not a valid inet address"}; }
		}

		auto value() const
		{ return m_value; }

		bool operator==(inet_address other) const
		{ return m_value.s_addr == other.m_value.s_addr; }

		bool operator!=(inet_address other) const
		{ return m_value.s_addr != other.m_value.s_addr; }

	private:
		in_addr m_value;
	};

	[[nodiscard]] inline auto to_string(inet_address addr)
	{
		auto const val = addr.value();
		std::array<char, 17> buffer{};
		inet_ntop(AF_INET, &val, std::data(buffer), 16);
		return std::string{std::data(buffer)};
	}

	[[nodiscard]] inline auto connect_to(inet_address address, uint16_t port)
	{
		auto socket = create_socket(AF_INET, SOCK_STREAM, 0);

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr = address.value();

		auto const res = ::connect(socket.get(),
			reinterpret_cast<sockaddr const*>(&addr),
			sizeof(addr));

		if(res == -1)
		{ throw system_error{"Failed to connect to server", errno};}

		return socket;
	}


	[[nodiscard]] inline auto try_bind(fd_ref socket, inet_address client_address, uint16_t port)
	{
		sockaddr_in sock_addr{};
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr = client_address.value();
		sock_addr.sin_port = htons(port);
		return ::bind(socket, reinterpret_cast<sockaddr const*>(&sock_addr), sizeof(sock_addr));
	}

	template<class BindFunc = decltype(try_bind)>
	inline auto bind(fd_ref socket,
		inet_address client_address,
		std::ranges::iota_view<int, int> ports_to_try,
		BindFunc&& bind_func = try_bind)
	{
		auto const i = std::ranges::find_if(ports_to_try,
			[socket, client_address, try_bind = std::forward<BindFunc>(bind_func)](auto port) {
			return try_bind(socket, client_address, static_cast<uint16_t>(port)) != -1;
		});

		if(i == std::end(ports_to_try))
		{ throw system_error{"Failed to bind socket", errno}; }

		return static_cast<uint16_t>(*i);
	}

	class inet_connection
	{
	public:
		explicit inet_connection(fd_owner fd, inet_address remote_address, uint16_t remote_port):
			m_fd{std::move(fd)},
			m_remote_address{remote_address},
			m_remote_port{remote_port},
			m_read_disabled{false}
		{ }

		[[nodiscard]] auto remote_port() const
		{ return m_remote_port; }

		[[nodiscard]] auto remote_address() const
		{ return m_remote_address; }

		void set_non_blocking()
		{ io::set_non_blocking(m_fd.get()); }

		[[nodiscard]] read_result read(std::span<char> buffer)
		{
			if(m_read_disabled)
			{
				return read_result{
					.bytes_read = static_cast<size_t>(0),
					.ec = operation_result::completed
				};
			}

			auto res = ::read(m_fd.get(), std::data(buffer), std::size(buffer));
			if(res == -1)
			{
				return read_result{
					.bytes_read = 0,
					.ec = (errno == EAGAIN || errno == EWOULDBLOCK)?
						operation_result::operation_would_block:
						operation_result::error
				};
			}

			return read_result{
				.bytes_read = static_cast<size_t>(res),
				.ec = operation_result::completed
			};
		}

		[[nodiscard]] write_result write(std::span<char const> buffer)
		{
			auto res = ::send(m_fd.get(), std::data(buffer), std::size(buffer), MSG_NOSIGNAL);
			if(res == -1)
			{
				return write_result{
					.bytes_written = 0,
					.ec = (errno == EAGAIN || errno == EWOULDBLOCK)?
						operation_result::operation_would_block:
						operation_result::error
				};
			}

			return write_result{
				.bytes_written = static_cast<size_t>(res),
				.ec = operation_result::completed
			};
		}

		void stop_reading()
		{
			::shutdown(m_fd.get(), SHUT_RD);
			m_read_disabled = true;
		}

		[[nodiscard]] fd_ref fd() const
		{ return m_fd.get(); }

	private:
		fd_owner m_fd;
		inet_address m_remote_address;
		uint16_t m_remote_port;
		bool m_read_disabled;
	};

	class inet_server_socket
	{
	public:
		explicit inet_server_socket(inet_address client_address,
			std::ranges::iota_view<int, int> ports_to_try,
			int listen_backlock,
			std::optional<int> conn_send_size = {}):
			m_fd{create_socket(AF_INET, SOCK_STREAM, 0)},
			m_conn_send_size{conn_send_size}
		{
			m_port = bind(m_fd.get(), client_address, ports_to_try);

			if(::listen(m_fd.get(), listen_backlock) == -1)
			{ throw system_error{"Failed to listen on socket", errno}; }
		}

		void set_non_blocking()
		{ io::set_non_blocking(m_fd.get()); }

		inet_connection accept() const
		{
			sockaddr_in client_addr{};
			socklen_t addr_length = sizeof(client_addr);
			fd_owner fd{::accept(m_fd.get(), reinterpret_cast<sockaddr*>(&client_addr), &addr_length)};
			if(fd.get() == nullptr)
			{ throw system_error{"Failed to establish a connection", errno}; }

			if(m_conn_send_size.has_value())
			{
				int send_size = *m_conn_send_size;
				auto res = setsockopt(fd.get(), SOL_SOCKET, SO_SNDBUF, &send_size, sizeof(send_size));
				if(res == -1)
				{ throw system_error{"Failed to set SO_SNDBUF", errno}; }
			}

			return inet_connection{
				std::move(fd),
				inet_address{client_addr.sin_addr},
				client_addr.sin_port
			};
		}

		[[nodiscard]] uint16_t port() const
		{ return m_port; }

		[[nodiscard]] fd_ref fd() const
		{ return m_fd.get(); }

	private:
		fd_owner m_fd;
		uint16_t m_port;
		std::optional<int> m_conn_send_size;
	};
}

#endif
