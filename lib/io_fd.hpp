#ifndef WEST_IO_FD_HPP
#define WEST_IO_FD_HPP

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <memory>
#include <functional>

namespace west::io
{
	struct fd
	{
		fd():value{-1}{}
		fd(int val) : value{val} {}
		fd(std::nullptr_t) : value{-1} {}

		operator int() const {return value;}

		bool operator ==(const fd& other) const = default;
		bool operator !=(const fd& other) const = default;

		bool operator ==(std::nullptr_t) const {return value == -1;}
		bool operator !=(std::nullptr_t) const {return value != -1;}

		operator bool() const { return value != -1; }

		int value;
	};

	struct fd_deleter
	{
		using pointer = fd;

		void operator()(fd desc) const
		{ ::close(desc); }
	};

	using fd_owner = std::unique_ptr<fd, fd_deleter>;

	template<class ... Args>
	[[nodiscard]] auto open(Args&& ... args)
	{ return fd_owner{::open(std::forward<Args>(args)...)}; }

	template<class ... Args>
	[[nodiscard]] auto create_socket(Args&& ... args)
	{ return fd_owner{::socket(std::forward<Args>(args)...)}; }

	inline void set_non_blocking(struct fd fd)
	{ ::fcntl(fd, F_SETFD, O_NONBLOCK); }
}

template<>
struct std::hash<west::io::fd>
{
	auto operator()(west::io::fd fd) const
	{ return std::hash<int>{}(fd); }
};

#endif