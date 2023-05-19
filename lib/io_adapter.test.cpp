//@	{"target": {"name": "io_adapter.test"}}

#include "./io_adapter.hpp"

#include "./utils.hpp"

#include <testfwk/testfwk.hpp>
#include <algorithm>

TESTCASE(west_ioadapter_bufferspan_initial_state)
{
	std::array<char, 60> buffer{};

	west::io_adapter::buffer_span const span{buffer};

	{
		auto const span_to_write = span.span_to_write();
		EXPECT_EQ(std::data(span_to_write), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write), std::size(buffer));

		auto const span_to_write_sliced_1 = span.span_to_write(std::size(buffer) - 1);
		EXPECT_EQ(std::data(span_to_write_sliced_1), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_1), std::size(buffer) - 1);

		auto const span_to_write_sliced_2 = span.span_to_write(std::size(buffer));
		EXPECT_EQ(std::data(span_to_write_sliced_2), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_2), std::size(buffer));

		auto const span_to_write_sliced_3 = span.span_to_write(std::size(buffer) + 1);
		EXPECT_EQ(std::data(span_to_write_sliced_3), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_3), std::size(buffer));
	}

	{
		auto const span_to_read = span.span_to_read();
		EXPECT_EQ(std::data(span_to_read), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read), 0);

		auto const span_to_read_sliced_1 = span.span_to_read(std::size(buffer) - 1);
		EXPECT_EQ(std::data(span_to_read_sliced_1), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read_sliced_1), 0);

		auto const span_to_read_sliced_2 = span.span_to_read(std::size(buffer));
		EXPECT_EQ(std::data(span_to_read_sliced_2), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read_sliced_2), 0);

		auto const span_to_read_sliced_3 = span.span_to_read(std::size(buffer) + 1);
		EXPECT_EQ(std::data(span_to_read_sliced_3), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read_sliced_3), 0);
	}
}

TESTCASE(west_io_adapter_bufferspan_reset_with_new_length_and_consume)
{
	static constexpr std::string_view str{"Aenean blandit erat nec odio congue, eu finibus nunc gravida"};		std::array<char, std::size(str)> buffer{};
	std::copy(std::begin(str), std::end(str), std::begin(buffer));

	west::io_adapter::buffer_span span{buffer};
	auto const new_size =(2*std::size(buffer))/3;
	span.reset_with_new_length(new_size);

	{
		auto const span_to_write = span.span_to_write();
		EXPECT_EQ(std::data(span_to_write), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write), std::size(buffer));

		auto const span_to_write_sliced_1 = span.span_to_write(std::size(buffer) - 1);
		EXPECT_EQ(std::data(span_to_write_sliced_1), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_1), std::size(buffer) - 1);

		auto const span_to_write_sliced_2 = span.span_to_write(std::size(buffer));
		EXPECT_EQ(std::data(span_to_write_sliced_2), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_2), std::size(buffer));

		auto const span_to_write_sliced_3 = span.span_to_write(std::size(buffer) + 1);
		EXPECT_EQ(std::data(span_to_write_sliced_3), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_3), std::size(buffer));
	}

	{
		auto const span_to_read = span.span_to_read();
		EXPECT_EQ(std::data(span_to_read), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read), new_size);

		auto const span_to_read_sliced_1 = span.span_to_read(new_size- 1);
		EXPECT_EQ(std::data(span_to_read_sliced_1), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read_sliced_1), new_size - 1);

		auto const span_to_read_sliced_2 = span.span_to_read(new_size);
		EXPECT_EQ(std::data(span_to_read_sliced_2), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read_sliced_2), new_size);

		auto const span_to_read_sliced_3 = span.span_to_read(new_size + 1);
		EXPECT_EQ(std::data(span_to_read_sliced_3), std::data(buffer));
		EXPECT_EQ(std::size(span_to_read_sliced_3), new_size);
	}

	auto const elements_to_consume = 10;
	span.consume_elements(elements_to_consume);

	{
		auto const span_to_write = span.span_to_write();
		EXPECT_EQ(std::data(span_to_write), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write), std::size(buffer));

		auto const span_to_write_sliced_1 = span.span_to_write(std::size(buffer) - 1);
		EXPECT_EQ(std::data(span_to_write_sliced_1), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_1), std::size(buffer) - 1);

		auto const span_to_write_sliced_2 = span.span_to_write(std::size(buffer));
		EXPECT_EQ(std::data(span_to_write_sliced_2), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_2), std::size(buffer));

		auto const span_to_write_sliced_3 = span.span_to_write(std::size(buffer) + 1);
		EXPECT_EQ(std::data(span_to_write_sliced_3), std::data(buffer));
		EXPECT_EQ(std::size(span_to_write_sliced_3), std::size(buffer));
	}

	{
		auto const span_to_read = span.span_to_read();
		EXPECT_EQ(std::data(span_to_read), std::data(buffer) + elements_to_consume);
		EXPECT_EQ(std::size(span_to_read), new_size - elements_to_consume);

		auto const span_to_read_sliced_1 = span.span_to_read(new_size - 1 - elements_to_consume);
		EXPECT_EQ(std::data(span_to_read_sliced_1), std::data(buffer) + elements_to_consume);
		EXPECT_EQ(std::size(span_to_read_sliced_1), new_size - 1 - elements_to_consume);

		auto const span_to_read_sliced_2 = span.span_to_read(new_size);
		EXPECT_EQ(std::data(span_to_read_sliced_2), std::data(buffer) + elements_to_consume);
		EXPECT_EQ(std::size(span_to_read_sliced_2), new_size - elements_to_consume);

		auto const span_to_read_sliced_3 = span.span_to_read(new_size + 1 - elements_to_consume);
		EXPECT_EQ(std::data(span_to_read_sliced_3), std::data(buffer) + elements_to_consume);
		EXPECT_EQ(std::size(span_to_read_sliced_3), new_size - elements_to_consume);
	}
}

namespace
{
	enum class read_error{exit, keep_going};
	enum class write_error{exit, keep_going};

	enum class retval{exit_by_write, exit_by_read, exit_at_end_of_buffer};

	template<class T>
	constexpr bool should_return(T ec)
	{ return ec == T::exit; }

	struct read_result
	{
		size_t bytes_read;
		read_error ec;
	};

	struct write_result
	{
		size_t bytes_written;
		write_error ec;
	};
}

TESTCASE(west_io_adapter_transfer_data_return_by_write)
{
	static constexpr std::string_view str{"Aenean blandit erat nec odio congue, eu finibus nunc gravida"};
	std::array<char, std::size(str)> buffer{};

	west::io_adapter::buffer_span span{buffer};
	auto bytes_left = std::size(buffer);
	std::string output_buffer;

	auto res = transfer_data([
			src_ptr = std::data(str),
			bytes_left = std::size(str)
		](std::span<char> buffer) mutable {
			auto bytes_to_read =std::min(static_cast<size_t>(13), bytes_left);
			std::copy_n(src_ptr, bytes_to_read, std::begin(buffer));
			bytes_left -= bytes_to_read;
			src_ptr += bytes_to_read;
			return read_result{bytes_to_read, read_error::keep_going};
		},
		[&output_buffer](std::span<char const> buffer){
			if(std::size(buffer) == 0)
			{ return write_result{0,  write_error::keep_going}; }
			EXPECT_EQ(std::size(buffer), 13);
			auto bytes_to_write = std::min(static_cast<size_t>(23), std::size(buffer));
			std::copy_n(std::data(buffer), bytes_to_write, std::back_inserter(output_buffer));
			return write_result{bytes_to_write,  write_error::exit};
		},
		west::overload{[](){return retval::exit_at_end_of_buffer;},
			[](read_error ec){
				return ec == read_error::exit ? retval::exit_by_read : retval::exit_at_end_of_buffer;
			},
			[](write_error ec){
				return ec ==write_error::exit ? retval::exit_by_write : retval::exit_at_end_of_buffer;
			}
		},
		span,
		bytes_left);

	EXPECT_EQ(res, retval::exit_by_write);
	EXPECT_EQ(output_buffer, "Aenean blandi");
	EXPECT_EQ(std::size(span.span_to_read()), 0);
	EXPECT_EQ(std::size(span.span_to_write()), std::size(buffer));
	EXPECT_EQ(bytes_left, std::size(buffer) - 13);
}

TESTCASE(west_io_adapter_transfer_data_return_by_read)
{
	static constexpr std::string_view str{"Aenean blandit erat nec odio congue, eu finibus nunc gravida"};
	std::array<char, std::size(str)> buffer{};

	west::io_adapter::buffer_span span{buffer};
	auto bytes_left = std::size(buffer);
	std::string output_buffer;

	auto res = transfer_data([
			src_ptr = std::data(str),
			bytes_left = std::size(str)
		](std::span<char> buffer) mutable {
			auto bytes_to_read =std::min(static_cast<size_t>(13), bytes_left);
			std::copy_n(src_ptr, bytes_to_read, std::begin(buffer));
			bytes_left -= bytes_to_read;
			src_ptr += bytes_to_read;
			return read_result{bytes_to_read, read_error::exit};
		},
		[&output_buffer](std::span<char const> buffer){
			if(std::size(buffer) == 0)
			{ return write_result{0,  write_error::keep_going}; }

			EXPECT_EQ(std::size(buffer), 13);
			auto bytes_to_write = std::min(static_cast<size_t>(23), std::size(buffer));
			std::copy_n(std::data(buffer), bytes_to_write, std::back_inserter(output_buffer));
			return write_result{bytes_to_write,  write_error::keep_going};
		},
		west::overload{[](){return retval::exit_at_end_of_buffer;},
			[](read_error ec){
				return ec == read_error::exit ? retval::exit_by_read : retval::exit_at_end_of_buffer;
			},
			[](write_error ec){
				return ec ==write_error::exit ? retval::exit_by_write : retval::exit_at_end_of_buffer;
			}
		},
		span,
		bytes_left);

	EXPECT_EQ(res, retval::exit_by_read);
	EXPECT_EQ(std::size(span.span_to_read()), 13);
	EXPECT_EQ(std::size(span.span_to_write()), std::size(buffer));
	EXPECT_EQ(bytes_left, std::size(buffer) - 13);
}