#ifndef WEST_IO_FD_HPP
#define WEST_IO_FD_HPP

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>

namespace west::io
{
	struct fd
	{
		fd():m_value{-1}{}
		fd(int value) : m_value{value} {}
		fd(std::nullptr_t) : m_value{-1} {}

		operator int() const {return m_value;}

		bool operator ==(const fd& other) const = default;
		bool operator !=(const fd& other) const = default;

		bool operator ==(std::nullptr_t) const {return m_value == -1;}
		bool operator !=(std::nullptr_t) const {return m_value != -1;}

		operator bool() const { return m_value != -1; }

		int m_value;
	};

	struct fd_deleter
	{
		using pointer = fd;

		void operator()(fd desc) const
		{ ::close(desc); }
	};

	using fd_owner = std::unique_ptr<fd, fd_deleter>;

	template<class ... Args>
	auto open(Args&& ... args)
	{ return fd_owner{::open(std::forward<Args>(args)...)}; }
}

#endif