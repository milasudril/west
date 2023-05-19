//@	{"target": {"name": "io_adapter.test"}}

#include "./io_adapter.hpp"

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