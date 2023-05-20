#ifndef WEST_IO_INTERFACES_HPP
#define WEST_IO_INTERFACES_HPP

#include <concepts>
#include <cstddef>
#include <span>

namespace west::io
{
	enum class operation_result{completed, operation_would_block, object_is_still_ready, error};

	constexpr bool should_return(operation_result res)
	{ return res != operation_result::object_is_still_ready; }

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