#ifndef WEST_IO_INTERFACES_HPP
#define WEST_IO_INTERFACES_HPP

#include <concepts>
#include <span>

namespace west::io
{
	template<class T, size_t N>
	class buffer_view
	{
	public:
		static constexpr auto buffer_size = N;

		explicit buffer_view(std::span<T, N> buffer):
			m_start{std::begin(buffer)},
			m_begin{std::begin(buffer)},
			m_end{std::begin(buffer)}
		{}

		std::span<T> span_to_write() const
		{ return std::span{m_start, m_start + N}; }

		void reset_with_new_end(T* new_end)
		{
			m_begin = m_start;
			m_end = new_end;
		}

		std::span<T const> span_to_read() const
		{ return std::span{m_begin, m_end}; }

		void consume_until(T* ptr)
		{ m_begin = ptr; }

	private:
		T* m_start;
		T* m_begin;
		T* m_end;
	};

	enum class operation_result{completed, operation_would_block, more_data_present, error};

	struct read_result
	{
		char* ptr;
		operation_result ec;
	};

	struct write_result
	{
		char const* ptr;
		operation_result ec;
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