#ifndef WEST_IO_SIGNALFD_HPP
#define WEST_IO_SIGNALFD_HPP

#include "./io_fd.hpp"

namespace west::io
{
	class signal_fd
	{
	public:
		explicit signal_fd(sigset_t sigmask):
			m_fd{create_signal_fd(sigmask)}
		{
			if (sigprocmask(SIG_BLOCK, &sigmask, nullptr) == -1)
			{ throw system_error{"Failed to capture signals", errno}; }
		}
		
		void set_non_blocking()
		{ io::set_non_blocking(m_fd.get()); }
		
		[[nodiscard]] read_result read(std::span<char> buffer)
		{
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
		
		[[nodiscard]] fd_ref fd() const
		{ return m_fd.get(); }
		
	private:
		fd_owner m_fd;
	};
};

#endif
