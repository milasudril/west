#ifndef WEST_IO_INET_SERVER_SOCKET_HPP
#define WEST_IO_INET_SERVER_SOCKET_HPP

#include "./io_fd.hpp"

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
		explicit inet_address(char const* str)
		{
			if(inet_aton(str, &m_value) == 0)
			{ throw std::runtime_error{"Not a valid inet address"}; }
		}

		auto value() const
		{ return m_value; }

	private:
		in_addr m_value;
	};

	inline auto try_bind(fd socket, inet_address client_address, uint16_t port)
	{
		sockaddr_in sock_addr{};
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr = client_address.value();
		sock_addr.sin_port = htons(port);
		return ::bind(socket, reinterpret_cast<sockaddr const*>(&sock_addr), sizeof(sock_addr));
	}

	template<class BindFunc = decltype(try_bind)>
	inline auto bind(fd socket,
		inet_address client_address,
		std::ranges::iota_view<int, int> ports_to_try,
		BindFunc&& bind_func = try_bind)
	{
		auto const i = std::ranges::find_if(ports_to_try,
			[socket, client_address, try_bind = std::forward<BindFunc>(bind_func)](auto port) {
			return try_bind(socket, client_address, static_cast<uint16_t>(port)) != -1;
		});

		if(i == std::end(ports_to_try))
		{
			throw std::runtime_error{
				std::string{"Failed to bind socket: "}
					.append(strerrordesc_np(errno))
			};
		}

		return *i;
	}
}

#endif