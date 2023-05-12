#ifndef WEST_IO_INTERFACES_HPP
#define WEST_IO_INTERFACES_HPP

namespace west::io
{
	enum class io_result{completed, more_data_needed, operation_would_block, error};

	struct read_result
	{
		char* ptr;
		io_result ec;
	};

	struct write_result
	{
		char const* ptr;
		io_result ec;
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