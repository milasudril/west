//@	{"target":{"name":"http_read_request_header.test"}}

#include "./http_session.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_read_request_header_initial_state)
{
	west::http::read_request_header reader;
	EXPECT_EQ(reader.get_content_length().has_value(), false);
	EXPECT_EQ(reader.get_keep_alive(), false);
}

namespace
{
	class data_source
	{
	public:
		explicit data_source(std::string_view buffer):
			m_buffer{buffer},
			m_ptr{std::begin(buffer)}
		{}

		auto read(std::span<char> output_buffer)
		{
			auto const remaining = std::size(m_buffer) - (m_ptr - std::begin(m_buffer));
			auto bytes_to_read =std::min(std::min(std::size(output_buffer), remaining), static_cast<size_t>(13));
			std::copy_n(m_ptr, remaining, std::begin(output_buffer));
			m_ptr += bytes_to_read;
			return west::io::read_result{
				.bytes_read = bytes_to_read,
				.ec = bytes_to_read == 0 ?
					west::io::operation_result::completed:
					west::io::operation_result::more_data_present
			};
		}

		char const* get_pointer() const { return m_ptr; }

	private:
		std::string_view m_buffer;
		char const* m_ptr;
	};

	struct request_handler
	{
		auto set_header(west::http::request_header&& header)
		{
			this->header = std::move(header);
			validation_result.http_status = west::http::status::i_am_a_teapot;
			validation_result.error_message = west::make_unique_cstr("This string comes from the test case");
			return std::move(validation_result);
		}

		west::http::header_validation_result validation_result;
		west::http::request_header header;
	};
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_nokeepalive_nocontentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n\r\nSome content after header"}};

	request_handler handler{};

	auto res = reader(buffer_view, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::i_am_a_teapot);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(reader.get_keep_alive(), false);
	EXPECT_EQ(reader.get_content_length().has_value(), false);
	auto const remaining_data = buffer_view.span_to_read();
	EXPECT_EQ((std::string_view{std::begin(remaining_data), std::end(remaining_data)}), "Some con");
	EXPECT_EQ(std::string_view{src.get_pointer()}, "tent after header");
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_nokeepalive_contentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"content-length: 234\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	auto res = reader(buffer_view, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::i_am_a_teapot);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(reader.get_keep_alive(), false);
	REQUIRE_EQ(reader.get_content_length().has_value(), true);
	EXPECT_EQ(*reader.get_content_length(), 234);
	EXPECT_EQ(std::string_view{src.get_pointer()}, "Some content after header");
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_keepalive_nocontentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: keep-alive\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	auto res = reader(buffer_view, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::i_am_a_teapot);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(reader.get_keep_alive(), false);
	EXPECT_EQ(reader.get_content_length().has_value(), false);
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_keepalivestrangevalue_contentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: foobar\r\n"
	"content-length: 123\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	auto res = reader(buffer_view, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::i_am_a_teapot);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(reader.get_keep_alive(), false);
	REQUIRE_EQ(reader.get_content_length().has_value(), true);
	EXPECT_EQ(*reader.get_content_length(), 123);
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_keepalive_contentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: keep-alive\r\n"
	"content-length: 123\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	auto res = reader(buffer_view, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::i_am_a_teapot);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(reader.get_keep_alive(), true);
	REQUIRE_EQ(reader.get_content_length().has_value(), true);
	EXPECT_EQ(*reader.get_content_length(), 123);
}