#ifndef WEST_UTILS_HPP
#define WEST_UTILS_HPP

#include <memory>
#include <cstring>

namespace west
{
	inline auto make_unique_cstr(char const* str)
	{
		auto n = strlen(str);
		auto ret = std::make_unique<char[]>(n + 1);
		memcpy(ret.get(), str, n + 1);
		return ret;
	}
}

#endif