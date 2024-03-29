#ifndef WEST_IOADAPTER_HPP
#define WEST_IOADAPTER_HPP

#include <span>
#include <array>
#include <cassert>
#include <utility>

namespace west::io_adapter
{
	template<class T>
	concept read_result = requires(T x)
	{
		{x.bytes_read} -> std::same_as<size_t&>;
		{x.ec};
	};

	template<class T>
	concept read_function = requires(T x, std::span<char> buffer)
	{
		{x(buffer)} -> read_result;
	};

	template<class T>
	concept write_result = requires(T x)
	{
		{x.bytes_written} -> std::same_as<size_t&>;
		{x.ec};
	};

	template<class T>
	concept write_function = requires(T x, std::span<char const> buffer, size_t bytes_to_read)
	{
		{x(buffer, bytes_to_read)} -> write_result;
	};

	template<class T, class ErrorCode>
	concept error_code_mapper = requires(T x, ErrorCode ec, size_t bytes_to_read)
	{
		{x(ec, bytes_to_read)};
		{x()};
	};

	template<class T, size_t BufferSize>
	class buffer_span
	{
	public:
		static constexpr auto buffer_size = BufferSize;

		explicit buffer_span(std::array<T, BufferSize>& buffer):
			m_start{std::data(buffer)},
			m_begin{std::data(buffer)},
			m_end{std::data(buffer)}
		{}

		std::span<T> span_to_write() const
		{ return std::span{m_start, m_start + BufferSize}; }

		std::span<T> span_to_write(size_t max_length) const
		{ return std::span{m_start, std::min(max_length, BufferSize)}; }

		void reset_with_new_length(size_t length)
		{
			assert(length <= BufferSize);
			m_begin = m_start;
			m_end = m_start + length;
		}

		std::span<T const> span_to_read() const
		{ return std::span{m_begin, m_end}; }

		std::span<T const> span_to_read(size_t max_length) const
		{ return std::span{m_begin, std::min(max_length, std::size(span_to_read()))}; }

		void consume_elements(size_t count)
		{
			assert(static_cast<size_t>(m_end - m_begin) >= count);
			m_begin += count;
		}

	private:
		T* m_start;
		T* m_begin;
		T* m_end;
	};

	namespace detail
	{
		template<class R>
		using read_function_res = std::invoke_result_t<R, std::span<char>>;

		template<class W>
		using write_function_res = std::invoke_result_t<W, std::span<char const>, size_t>;

		template<class Result>
		using ec_type = std::remove_cvref_t<decltype(std::declval<Result>().ec)>;
	}

	template<class T>
	struct error_code_checker
	{
		bool operator()(T ec) const
		{ return is_error_indicator(ec); }
	};

	template<read_function R, write_function W, class ErrorCodeMapper, size_t BufferSize>
	requires(error_code_mapper<ErrorCodeMapper, detail::ec_type<detail::read_function_res<R>>>
		&& error_code_mapper<ErrorCodeMapper, detail::ec_type<detail::write_function_res<W>>>)
	auto transfer_data(R&& read,
		W&& write,
		ErrorCodeMapper&& map_error_code,
		buffer_span<char, BufferSize>& buffer, size_t& bytes_to_read)
	{
		while(bytes_to_read != 0)
		{
			if(std::size(buffer.span_to_read()) == 0)
			{
				auto span_to_write = buffer.span_to_write();
				auto const read_result = read(span_to_write);
				buffer.reset_with_new_length(read_result.bytes_read);

				using ec_type = detail::ec_type<detail::read_function_res<R>>;

				if(error_code_checker<ec_type>{}(read_result.ec) || read_result.bytes_read == 0)
				{ return map_error_code(read_result.ec, std::as_const(bytes_to_read)); }
			}
			else
			{
				auto const span_to_read = buffer.span_to_read(bytes_to_read);
				auto const write_result = write(span_to_read, std::as_const(bytes_to_read));
				buffer.consume_elements(write_result.bytes_written);
				bytes_to_read -= write_result.bytes_written;

				using ec_type = detail::ec_type<detail::write_function_res<W>>;

				if(error_code_checker<ec_type>{}(write_result.ec) || write_result.bytes_written == 0)
				{ return map_error_code(write_result.ec, std::as_const(bytes_to_read)); }
			}
		}
		return map_error_code();
	}
}
#endif
