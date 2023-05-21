//@	{"target":{"name":"http_session.test.cpp"}}

#include "./http_session.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_session_make_read_resopnse)
{
	{
		bool called = false;
		auto const res = west::http::make_read_response(west::io::operation_result::completed,
			[&called](){
				called = true;
				return "Hello, World";
			});
		EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
		EXPECT_EQ(res.http_status, west::http::status::bad_request);
		EXPECT_EQ(res.error_message.get(), std::string_view{"Hello, World"});
		EXPECT_EQ(called, true);
	}

	{
		bool called = false;
		auto const res = west::http::make_read_response(west::io::operation_result::operation_would_block,
			[&called](){
				called = true;
				return "Hello, World";
			});
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
		EXPECT_EQ(called, false);
	}

	{
		bool called = false;
		auto const res = west::http::make_read_response(west::io::operation_result::error,
			[&called](){
				called = true;
				return "Hello, World";
			});
		EXPECT_EQ(res.status, west::http::session_state_status::io_error);
		EXPECT_EQ(res.http_status, west::http::status::internal_server_error);
		EXPECT_EQ(res.error_message.get(), std::string_view{"I/O error"});
		EXPECT_EQ(called, false);
	}
}

TESTCASE(west_http_session_make_write_resopnse)
{
	{
		auto const res = west::http::make_write_response(west::io::operation_result::completed);
		EXPECT_EQ(res.status, west::http::session_state_status::connection_closed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
	}

	{
		auto const res = west::http::make_write_response(west::io::operation_result::operation_would_block);
		EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
		EXPECT_EQ(res.http_status, west::http::status::ok);
		EXPECT_EQ(res.error_message.get(), nullptr);
	}

	{
		auto const res = west::http::make_write_response(west::io::operation_result::error);
		EXPECT_EQ(res.status, west::http::session_state_status::io_error);
		EXPECT_EQ(res.http_status, west::http::status::internal_server_error);
		EXPECT_EQ(res.error_message.get(), std::string_view{"I/O error"});
	}
}