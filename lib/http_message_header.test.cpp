//@	{"target":{"name":"http_message_header.test"}}

#include "./http_message_header.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_version)
{
	west::http::version ver{};
	EXPECT_EQ(ver.major(), 0);
	EXPECT_EQ(ver.minor(), 0);

	ver.major(123);
	EXPECT_EQ(ver.major(), 123);
	EXPECT_EQ(ver.minor(), 0);

	ver.minor(456);
	EXPECT_EQ(ver.major(), 123);
	EXPECT_EQ(ver.minor(), 456);

	ver.major(45).minor(7);
	EXPECT_EQ(ver.major(), 45);
	EXPECT_EQ(ver.minor(), 7);

	EXPECT_EQ(ver, (west::http::version{45, 7}));

	auto str = to_string(ver);
	EXPECT_EQ(str, "45.7");
}
