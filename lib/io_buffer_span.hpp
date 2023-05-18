#ifndef WEST_IO_BUFFER_SPAN_HPP
#define WEST_IO_BUFFER_SPAN_HPP

#include <span>
#include <cassert>

namespace west::io
{
	template<class T, size_t BufferSize>
	class buffer_span
	{
	public:
		static constexpr auto buffer_size = BufferSize;

		explicit buffer_span(std::array<T, BufferSize>& buffer):
			buffer_span{std::span{buffer}}
		{}

		explicit buffer_span(std::span<T, BufferSize> buffer):
			m_start{std::data(buffer)},
			m_begin{std::data(buffer)},
			m_end{std::data(buffer)}
		{
			assert(m_begin != nullptr);
		}

		std::span<T> span_to_write() const
		{ return std::span{m_start, m_start + BufferSize}; }

		std::span<T> span_to_write(size_t max_length) const
		{ return std::span{m_start, std::min(max_length, BufferSize)}; }

		void reset_with_new_length(size_t length)
		{
			m_begin = m_start;
			assert(m_begin != nullptr);
			m_end = m_start + length;
		}

		std::span<T const> span_to_read() const
		{ return std::span{m_begin, m_end}; }

		std::span<T const> span_to_read(size_t max_length) const
		{ return std::span{m_begin, std::min(max_length, std::size(span_to_read()))}; }

		void consume_elements(size_t count)
		{
			m_begin += count;
			assert(m_begin != nullptr);
		}

	private:
		T* m_start;
		T* m_begin;
		T* m_end;
	};
}

#endif