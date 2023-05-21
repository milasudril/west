#ifndef WEST_STUBS_DATASOURCE_HPP
#define WEST_STUBS_DATASOURCE_HPP

#include "lib/io_interfaces.hpp"

#include <string_view>
#include <span>

namespace west::stubs
{
	class data_source
	{
	public:
		enum class mode{normal, blocking, error};

		explicit data_source(std::string_view buffer):
			m_buffer{buffer},
			m_ptr{std::begin(buffer)},
			m_mode{mode::normal}
		{}

		auto read(std::span<char> output_buffer)
		{
			switch(m_mode)
			{
				case mode::normal:
				{
					auto const remaining = std::size(m_buffer) - (m_ptr - std::begin(m_buffer));
					auto bytes_to_read =std::min(std::min(std::size(output_buffer), remaining), static_cast<size_t>(13));
					std::copy_n(m_ptr, bytes_to_read, std::begin(output_buffer));
					m_ptr += bytes_to_read;
					return west::io::read_result{
						.bytes_read = bytes_to_read,
						.ec = bytes_to_read == 0 ?
							west::io::operation_result::completed:
							west::io::operation_result::object_is_still_ready
					};
				}

				case mode::blocking:
					return west::io::read_result{
						.bytes_read = 0,
						.ec = west::io::operation_result::operation_would_block
					};

				case mode::error:
					return west::io::read_result{
						.bytes_read = 0,
						.ec = west::io::operation_result::error
					};
				default:
					__builtin_unreachable();
			}
		}

		char const* get_pointer() const { return m_ptr; }

		void set_mode(mode new_mode)
		{ m_mode = new_mode; }

		auto get_data() const { return m_buffer; }


	private:
		std::string_view m_buffer;
		char const* m_ptr;
		mode m_mode;
	};
}

#endif