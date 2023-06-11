//@	{"target":{"name":"http_read_request_header.test"}}

#include "./http_read_request_header.hpp"

#include "stubs/data_source.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct request_handler
	{
		west::http::status return_status{west::http::status::accepted};

		auto finalize_state(west::http::request_header const&) const
		{
			west::http::finalize_state_result validation_result;
			validation_result.http_status = return_status;
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

	west::stubs::data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"
"Some content after header"}};
	src.read(std::span{std::data(buffer), 3});
	buff_span.reset_with_new_length(3);

	west::http::session session{src, request_handler{}, west::http::request_info{}, west::http::response_header{}};
	auto res = reader.socket_is_ready(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.state_result.http_status, west::http::status::accepted);
	EXPECT_EQ(res.state_result.error_message.get(), std::string_view{"This string comes from the test case"});
	EXPECT_EQ(session.request_info.header.request_line.method, "GET");
	EXPECT_EQ(session.request_info.header.request_line.request_target, "/");
	EXPECT_EQ(session.request_info.header.request_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.request_info.header.fields.find("host")->second, "localhost:80");
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ((std::string_view{std::begin(remaining_data), std::end(remaining_data)}), "Some");
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, " content after header");
}

TESTCASE(west_http_read_request_header_read_incomplete_at_eof)
{
	west::http::read_request_header reader;

	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r"}};

	west::http::session session{src, request_handler{}, west::http::request_info{}, west::http::response_header{}};
	auto res = reader.socket_is_ready(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.state_result.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.state_result.error_message.get(), std::string_view{"Header truncated"});
}

TESTCASE(west_http_read_request_header_read_unsupported_http_version)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{"GET / HTTP/1.5\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_info{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader.socket_is_ready(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.state_result.http_status, west::http::status::http_version_not_supported);
	EXPECT_EQ(res.state_result.error_message.get(),
		std::string_view{"This web server only supports HTTP version 1.1"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 0);
}

TESTCASE(west_http_read_request_header_read_successful_fail_in_req_handler)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{west::http::status::not_found}, west::http::request_info{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader.socket_is_ready(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.state_result.http_status, west::http::status::not_found);
	EXPECT_EQ(res.state_result.error_message.get(), std::string_view{"This string comes from the test case"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 0);
}

TESTCASE(west_http_read_request_header_read_bad_header)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{"GET / FTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_info{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer())};
	auto res = reader.socket_is_ready(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.state_result.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.state_result.error_message.get(), std::string_view{"Wrong protocol"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 3);
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "\r\n"
"host: localhost:80\r\n\r\n");
}

TESTCASE(west_http_read_request_header_read_oversized_header)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{"GET / HTTP/1.1\r\n"
"host: localhost:80\r\n\r\n"}};

	west::http::session session{src, request_handler{}, west::http::request_info{}, west::http::response_header{}};
	west::http::read_request_header reader{strlen(src.get_pointer()) - 3};
	auto res = reader.socket_is_ready(buff_span, session);

	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.state_result.http_status, west::http::status::request_entity_too_large);
	EXPECT_EQ(res.state_result.error_message.get(), std::string_view{"Header truncated"});
	auto const remaining_data = buff_span.span_to_read();
	EXPECT_EQ(std::size(remaining_data), 3);
	EXPECT_EQ(std::string_view{session.connection.get_pointer()}, "");
}