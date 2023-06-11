#ifndef WEST_IO_FD_HPP
#define WEST_IO_FD_HPP

#include "./system_error.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <memory>
#include <functional>

namespace west::io
{
	struct fd_ref
	{
		fd_ref():value{-1}{}

		template<class T>
		requires std::is_same_v<T, int>
		fd_ref(T val) : value{val} {}

		fd_ref(std::nullptr_t) : value{-1} {}

		operator int() const {return value;}

		bool operator ==(const fd_ref& other) const = default;
		bool operator !=(const fd_ref& other) const = default;

		bool operator ==(std::nullptr_t) const {return value == -1;}
		bool operator !=(std::nullptr_t) const {return value != -1;}

		operator bool() const { return value != -1; }

		int value;
	};

	struct fd_deleter
	{
		using pointer = fd_ref;

		void operator()(fd_ref desc) const
		{ ::close(desc); }
	};

	using fd_owner = std::unique_ptr<fd_ref, fd_deleter>;

	template<class ... Args>
	[[nodiscard]] auto open(Args&& ... args)
	{
		auto tmp = ::open(std::forward<Args>(args)...);
		if(tmp == -1)
		{ throw system_error{"Failed to open file", errno}; }

		return fd_owner{fd_ref{tmp}};
	}

	template<class ... Args>
	[[nodiscard]] auto create_socket(Args&& ... args)
	{
		auto tmp = ::socket(std::forward<Args>(args)...);
		if(tmp == -1)
		{ throw system_error{"Failed to create socket", errno}; }

		return fd_owner{fd_ref{tmp}};
	}

	struct pipe
	{
		fd_owner read_end;
		fd_owner write_end;
	};

	[[nodiscard]] inline auto create_pipe(int flags)
	{
		std::array<int, 2> fds{};
		if( pipe2(std::data(fds), flags) == -1)
		{ throw system_error{"Failed to create a pipe", errno}; }

		return pipe{
			.read_end = fd_owner{fd_ref{fds[0]}},
			.write_end = fd_owner{fd_ref{fds[1]}}
		};
	}

	inline void set_non_blocking(fd_ref fd)
	{
		if(::fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		{ throw system_error{"Failed to enable nonblocking mode", errno};}
	}
}

template<>
struct std::hash<west::io::fd_ref>
{
	auto operator()(west::io::fd_ref fd) const
	{ return std::hash<int>{}(fd); }
};

#endif
