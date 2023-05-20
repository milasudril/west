//@	{"target":{"name":"http_read_request_header.test"}}

#include "./http_read_request_header.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	class data_source
	{
	public:
		enum class mode{normal, blocking, error};

		explicit data_source(std::string_view buffer):
			m_buffer{buffer},
			m_ptr{std::begin(buffer)},
			m_mode{mode::normal}
		{}

		auto read(std::span<char> output_buffer)
		{
			switch(m_mode)
			{
				case mode::normal:
				{
					auto const remaining = std::size(m_buffer) - (m_ptr - std::begin(m_buffer));
					auto bytes_to_read =std::min(std::min(std::size(output_buffer), remaining), static_cast<size_t>(13));
					std::copy_n(m_ptr, bytes_to_read, std::begin(output_buffer));
					m_ptr += bytes_to_read;
					return west::io::read_result{
						.bytes_read = bytes_to_read,
						.ec = bytes_to_read == 0 ?
							west::io::operation_result::completed:
							west::io::operation_result::object_is_still_ready
					};
				}

				case mode::blocking:
					return west::io::read_result{
						.bytes_read = 0,
						.ec = west::io::operation_result::operation_would_block
					};

				case mode::error:
					return west::io::read_result{
						.bytes_read = 0,
						.ec = west::io::operation_result::error
					};
				default:
					__builtin_unreachable();
			}
		}

		char const* get_pointer() const { return m_ptr; }

		void set_mode(mode new_mode)
		{ m_mode = new_mode; }


	private:
		std::string_view m_buffer;
		char const* m_ptr;
		mode m_mode;
	};

	struct request_handler
	{
		auto finalize_state(west::http::request_header const&) const
		{
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::accepted;
			validation_result.error_message = west::make_unique_cstr("This string comes from the test case");
			return validation_result;
		}
	};
}

TESTCASE(west_http_read_request_header_read_successful_with_data_after_header)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"
"Some content after header"}};
	src.read(std::span{std::data(buffer), 3});
	buff_span.reset_with_new_length(3);

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(session.request_header.request_line.method, "GET");
	EXPECT_EQ(session.request_header.request_line.request_target, "/");
	EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.request_header.fields.find("host")->second, "localhost:80");
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ((std::string_view{std::begin(remaining_data), std::end(remaining_data)}), "Some");
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, " content after header");
}

TESTCASE(west_http_read_request_header_read_complete_at_eof)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(session.request_header.request_line.method, "GET");
	EXPECT_EQ(session.request_header.request_line.request_target, "/");
	EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.request_header.fields.find("host")->second, "localhost:80");
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 0);
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "");
}

TESTCASE(west_http_read_request_header_read_incomplete_at_eof)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Header truncated"});
}

TESTCASE(west_http_read_request_header_read_complete_header_limited_size)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\nFoo"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer()) - 3};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(session.request_header.request_line.method, "GET");
	EXPECT_EQ(session.request_header.request_line.request_target, "/");
	EXPECT_EQ(session.request_header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.request_header.fields.find("host")->second, "localhost:80");
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 1);
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "oo");
}

TESTCASE(west_http_read_request_header_read_oversized_header)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer()) - 3};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::request_entity_too_large);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Header truncated"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 3);
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "");
}

TESTCASE(west_http_read_request_header_read_operation_would_block)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};
	src.set_mode(data_source::mode::blocking);

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
	EXPECT_EQ(res.http_status, west::http::status::ok);
	EXPECT_EQ(res.error_message.get(), nullptr);
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 0);
}

TESTCASE(west_http_read_request_header_read_io_error)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};
	src.set_mode(data_source::mode::error);

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::io_error);
	EXPECT_EQ(res.http_status, west::http::status::internal_server_error);
	EXPECT_EQ(res.error_message.get(), std::string_view{"I/O error"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 0);
}

TESTCASE(west_http_read_request_header_read_unsupported_http_version)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / HTTP/1.5\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::http_version_not_supported);
	EXPECT_EQ(res.error_message.get(), std::string_view{"This web server only supports HTTP version 1.1"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 0);
}

TESTCASE(west_http_read_request_header_read_bad_header)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	data_source src{std::string_view{"GET / FTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_header{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Wrong protocol"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 3);
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "\r\n"
"host: localhost:80\r\n\r\n");
}
