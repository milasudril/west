//	{"target":{"name":"http_read_request_header.test"}}

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
		auto finalize_state(west::http::read_request_header_tag, west::http::request_header const&) const
		{
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::accepted;
			validation_result.error_message = west::make_unique_cstr("This string comes from the test case");
			return validation_result;
		}
	};
}

TESTCASE(west_http_read_request_header_read_noblocking_noparseerror_notruncation)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n\r\nSome content after header"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}};
	auto res = reader(buffer_view, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{1, 1}));
	auto const remaining_data = buffer_view.span_to_read();
	EXPECT_EQ((std::string_view{std::begin(remaining_data), std::end(remaining_data)}), "Some con");
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "tent after header");
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

	west::http::session session{src, request_handler{}, west::http::request_header{}};
	auto res = reader(buffer_view, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Wrong protocol"});
	EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
}

TESTCASE(west_http_read_request_header_read_noblocking_truncation)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
	"connection: ke"}};


	west::http::session session{src, request_handler{}, west::http::request_header{}};
	auto res = reader(buffer_view, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"More data needed"});
	EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
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

	west::http::session session{src, request_handler{}, west::http::request_header{}};

	{
		auto res = reader(buffer_view, session);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
	}

	{
		auto res = reader(buffer_view, session);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
	}

	{
		auto res = reader(buffer_view, session);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
	}

	{
		auto res = reader(buffer_view, session);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
	}

	{
		auto res = reader(buffer_view, session);
		EXPECT_EQ(res.status, west::http::session_state_status::completed);
		EXPECT_EQ(res.http_status, west::http::status::accepted);
		EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
		EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{1, 1}));
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

	{
		west::http::session session{src, request_handler{}, west::http::request_header{}};
		auto res = reader(buffer_view, session);
		EXPECT_EQ(res.status, west::http::session_state_status::io_error);
		EXPECT_EQ(res.http_status, west::http::status::internal_server_error);
		EXPECT_EQ(res.error_message.get(),  std::string_view{"I/O error"});
		EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{0, 0}));
	}
}