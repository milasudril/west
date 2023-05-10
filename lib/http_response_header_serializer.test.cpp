//@	{"target":{"name":"http_response_header_serializer.test"}}

#include "./http_response_header_serializer.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_response_header_serializer_serialize_no_reason_phrase)
{
	west::http::response_header response{};
	response.status_line.http_version = west::http::version{1, 1};
	response.status_line.status_code = west::http::status::i_am_a_teapot;
	response.fields
		.append(*west::http::field_name::create("connection"), *west::http::field_value::create("closed"))
		.append(*west::http::field_name::create("content-type"), *west::http::field_value::create("text/plain"));

	west::http::response_header_serializer serializer{response};

	std::array<char, 13> buffer{};
	std::string result;
	while(true)
	{
		auto const res = serializer.serialize(buffer);
		result.insert(std::end(result), std::data(buffer), res.ptr);

		if(res.ec == west::http::resp_header_serializer_error_code::completed)
		{ break; }
	}

	EXPECT_EQ(result, std::string_view{"HTTP/1.1 418 I am a teapot\r\n"
"connection: closed\r\n"
"content-type: text/plain\r\n"
"\r\n"});
}