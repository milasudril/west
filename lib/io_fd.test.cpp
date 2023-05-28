//@	{"target":{"name":"io_fd.test"}}

#include "./io_fd.hpp"

#include <testfwk/testfwk.hpp>

#include <filesystem>

TESTCASE(west_io_fd_create_file)
{
	auto const filename  = std::filesystem::path{} /
		MAIKE_BUILDINFO_TARGETDIR /
		std::string{"task_"}
			.append(std::to_string(MAIKE_TASKID))
			.append("_create_file");
	{
		auto const file = west::io::open(filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
		EXPECT_NE(file, nullptr);
		auto const n = ::write(file.get(), "Hello, World", 12);
		EXPECT_EQ(n, 12);
	}

	{
		auto const file = west::io::open(filename.c_str(), O_RDONLY);
		EXPECT_NE(file, nullptr);
		std::array<char, 12> buffer{};
		auto const n = ::read(file.get(), buffer.data(), 12);
		EXPECT_EQ(n, 12);
		EXPECT_EQ((std::string_view{buffer.data(), 12}), "Hello, World");
	}
}