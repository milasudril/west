//@	{"target":{"name":"utils.test"}}

#include "./utils.hpp"

#include <cstdint>

#include <testfwk/testfwk.hpp>

TESTCASE(west_utils_to_number)
{
	EXPECT_EQ(west::to_number<uint32_t>("").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("-124").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("4294967296434").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("0xff").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("  4").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("4aser").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("4\r\n").has_value(), false);
	EXPECT_EQ(west::to_number<uint32_t>("2"), 2);
}
