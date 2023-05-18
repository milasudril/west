#ifndef WEST_IO_INTERFACES_HPP
#define WEST_IO_INTERFACES_HPP

#include <concepts>
#include <span>

#include <cassert>

namespace west::io
{
	template<class T, size_t N>
	class buffer_span
	{
	public:
		static constexpr auto buffer_size = N;

		explicit buffer_span(std::array<T, N>& buffer):
			buffer_span{std::span{buffer}}
		{}

		explicit buffer_span(std::span<T, N> buffer):
			m_start{std::data(buffer)},
			m_begin{std::data(buffer)},
			m_end{std::data(buffer)}
		{
			assert(m_begin != nullptr);
		}

		std::span<T> span_to_write() const
		{ return std::span{m_start, m_start + N}; }

		void reset_with_new_length(size_t length)
		{
			m_begin = m_start;
			assert(m_begin != nullptr);
			m_end = m_start + length;
		}

		std::span<T const> span_to_read() const
		{ return std::span{m_begin, m_end}; }

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

	enum class operation_result{completed, operation_would_block, more_data_present, error};

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

	template<class T>
	concept socket = requires(T x, std::span<char> y, std::span<char const> z)
	{
		{x.read(y)} -> std::same_as<read_result>;
		{x.write(z)} -> std::same_as<write_result>;
		{x.stop_reading()} -> std::same_as<void>;
	};
}

#endif