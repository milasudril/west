#ifndef WEST_IO_INTERFACES_HPP
#define WEST_IO_INTERFACES_HPP

#include <concepts>
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

	enum class operation_result{completed, operation_would_block, object_is_still_ready, error};

	struct read_result
	{
		size_t bytes_read;
		operation_result ec;
	};

	template<class T>
	concept data_source = requires(T x, std::span<char> y)
	{
		{x.read(y)} -> std::same_as<read_result>;
	};

	struct write_result
	{
		size_t bytes_written;
		operation_result ec;
	};

	template<class T>
	concept data_sink = requires(T x, std::span<char const> y)
	{
		{x.write(y)} -> std::same_as<write_result>;
	};

	template<data_source Source, data_sink Sink, size_t BufferSize>
	auto transfer_data(Source& from, Sink& to, buffer_span<char, BufferSize>& buffer, size_t& bytes_left)
	{
		while(bytes_left != 0)
		{
			auto const span_to_read = buffer.span_to_read(bytes_left);
			auto const write_result = to.write(span_to_read);
			buffer.consume_elements(write_result.bytes_written);
			bytes_left -= write_result.bytes_written;

			if(should_return(write_result.ec))
			{ return make_data_transfer_result(write_result.ec); }

			if(std::size(buffer.span_to_read()) == 0 && bytes_left != 0)
			{
				auto span_to_write = buffer.span_to_write(bytes_left);
				auto const read_result = from.read(span_to_write);
				buffer.reset_with_new_length(read_result.bytes_read);

				if(should_return(read_result.ec))
				{ return make_data_transfer_result(read_result.ec); }
			}
		}

		// FIXME return value here
	}

	template<class T>
	concept socket = requires(T x, std::span<char> y, std::span<char const> z)
	{
		{x.read(y)} -> std::same_as<read_result>;
		{x.write(z)} -> std::same_as<write_result>;
		{x.stop_reading()} -> std::same_as<void>;
	};
}

#endif