//@	{"target":{"name":"http_read_request_header.test"}}

#include "./http_read_request_header.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	class data_source
	{
	public:
		explicit data_source(std::string_view buffer):
			m_buffer{buffer},
			m_ptr{std::begin(buffer)},
			m_callcount{0},
			m_blocking_enabled{false},
			m_error_enabled{false}
		{}

		auto read(std::span<char> output_buffer)
		{
			++m_callcount;
			if(m_callcount % 2 == 0 && m_blocking_enabled)
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::operation_would_block
				};
			}

			if(m_callcount % 3 == 0 && m_error_enabled)
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::error
				};
			}

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

		void toggle_blocking()
		{ m_blocking_enabled = !m_blocking_enabled; }

		void toggle_error()
		{ m_error_enabled = !m_error_enabled; }

	private:
		std::string_view m_buffer;
		char const* m_ptr;
		size_t m_callcount;
		bool m_blocking_enabled;
		bool m_error_enabled;
	};

	struct request_handler
	{
		auto set_header(west::http::request_header&& header)
		{
			this->header = std::move(header);
			validation_result.http_status = west::http::status::accepted;
			validation_result.error_message = west::make_unique_cstr("This string comes from the test case");
			return std::move(validation_result);
		}

		west::http::header_validation_result validation_result;
		west::http::request_header header;
	};
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_noclose_nocontentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n\r\nSome content after header"}};

	request_handler handler{};

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.conn_keep_alive, true);
	EXPECT_EQ(session.req_content_length, 0);
	auto const remaining_data = buffer_view.span_to_read();
	EXPECT_EQ((std::string_view{std::begin(remaining_data), std::end(remaining_data)}), "Some con");
	EXPECT_EQ(std::string_view{src.get_pointer()}, "tent after header");
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_noclose_contentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"content-length: 234\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.conn_keep_alive, true);
	EXPECT_EQ(session.req_content_length, 234);
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

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.conn_keep_alive, true);
	EXPECT_EQ(session.req_content_length, 0);
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_closestrangevalue_contentlength)
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

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.conn_keep_alive, true);
	EXPECT_EQ(session.req_content_length, 123);
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation_close_contentlength)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: close\r\n"
	"content-length: 123\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.conn_keep_alive, false);
	EXPECT_EQ(session.req_content_length, 123);
}

TESTCASE(west_http_read_request_header_read_noblocking_parseerror_notruncation)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / FTP/1.1\r\n"
	"connection: keep-alive\r\n"
	"content-length: 123\r\n"
	"\r\n"
	"Some content after header"}};

	request_handler handler{};

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Wrong protocol"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
	EXPECT_EQ(session.conn_keep_alive, true);
	EXPECT_EQ(session.req_content_length, 0);
}

TESTCASE(west_http_read_request_header_read_noblocking_truncation)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: ke"}};

	request_handler handler{};

	west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"More data needed"});
	EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
	EXPECT_EQ(session.conn_keep_alive, true);
	EXPECT_EQ(session.req_content_length, 0);
}

TESTCASE(west_http_read_request_header_read_blocking)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: close\r\n"
	"content-length: 123\r\n"
	"\r\n"
	"Some content after header"}};
	src.toggle_blocking();

	request_handler handler{};

	{
		west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
		EXPECT_EQ(session.conn_keep_alive, true);
		EXPECT_EQ(session.req_content_length, 0);
	}

	{
		west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
		EXPECT_EQ(session.conn_keep_alive, true);
		EXPECT_EQ(session.req_content_length, 0);
	}

	{
		west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
		EXPECT_EQ(session.conn_keep_alive, true);
		EXPECT_EQ(session.req_content_length, 0);
	}

	{
		west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
		EXPECT_EQ(session.conn_keep_alive, true);
		EXPECT_EQ(session.req_content_length, 0);
	}

	{
		west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);
		EXPECT_EQ(res.status, west::http::session_state_status::completed);
		EXPECT_EQ(res.http_status, west::http::status::accepted);
		EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
		EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{1, 1}));
		EXPECT_EQ(session.conn_keep_alive, false);
		EXPECT_EQ(session.req_content_length, 123);
	}
}


TESTCASE(west_http_read_request_header_read_io_error)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: close\r\n"
	"content-length: 123\r\n"
	"\r\n"
	"Some content after header"}};
	src.toggle_error();

	request_handler handler{};

	{
		west::http::session session{};
	auto res = reader(buffer_view, session, src, handler);
		EXPECT_EQ(res.status, west::http::session_state_status::io_error);
		EXPECT_EQ(res.http_status, west::http::status::internal_server_error);
		EXPECT_EQ(res.error_message.get(),  std::string_view{"I/O error"});
		EXPECT_EQ(handler.header.request_line.http_version, (west::http::version{0, 0}));
		EXPECT_EQ(session.conn_keep_alive, true);
		EXPECT_EQ(session.req_content_length, 0);
	}
}