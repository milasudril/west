#ifndef WEST_SYSTEM_ERROR_HPP
#define WEST_SYSTEM_ERROR_HPP

#include <stdexcept>
#include <cstring>

namespace west
{
	class system_error:public std::runtime_error
	{
	public:
		explicit system_error(std::string msg, int err):
			runtime_error{msg.append(": ").append(strerrordesc_np(err))}
		{}
	};
}

#endif