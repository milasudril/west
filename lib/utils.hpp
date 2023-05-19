#ifndef WEST_UTILS_HPP
#define WEST_UTILS_HPP

#include <memory>
#include <cstring>
#include <charconv>
#include <optional>

namespace west
{
	inline auto make_unique_cstr(char const* str)
	{
		auto n = strlen(str);
		auto ret = std::make_unique<char[]>(n + 1);
		memcpy(ret.get(), str, n + 1);
		return ret;
	}

	template<class T>
	requires(std::is_arithmetic_v<T>)
	inline std::optional<T> to_number(std::string_view val)
	{
		if(std::begin(val) == std::end(val))
		{ return std::nullopt; }

		T ret{};
		auto res = std::from_chars(std::begin(val), std::end(val), ret);
		if(res.ptr != std::end(val) || res.ec == std::errc::result_out_of_range)
		{ return std::nullopt; }

		return ret;
	}

	template<class ... Args>
	struct overload : Args...
	{
		using Args::operator() ...;
	};
}

#endif