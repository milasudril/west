//@	{"target":{"name":"http_request_header_parser.test"}}

#include "./http_request_header_parser.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_request_header_parser_parse_complete_header)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/111.0\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;\r\n"
"\tq=0.9,image/avif,image/webp,*/*;\r\n"
"  q=0.8\r\n"
"Accept-Language: sv-SE,sv;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"DNT: 1\r\n"
"Connection: keep-alive\r\n"
"Upgrade-Insecure-Requests: 1\r\n"
"Sec-Fetch-Dest: document\r\n"
"Sec-Fetch-Mode: navigate\r\n"
"Sec-Fetch-Site: none\r\n"
"Sec-Fetch-User: ?1\r\n"
"\r\nSome additional data"};

	west::http::request_header header{};
	west::http::request_header_parser parser{header};

	auto res = parser.parse(serialized_header);
	EXPECT_EQ(res.ec, west::http::req_header_parser_error_code::completed);

	EXPECT_EQ(header.request_line.method, "GET");
	EXPECT_EQ(header.request_line.request_target, "/");
	EXPECT_EQ(header.request_line.http_version.major(), 1);
	EXPECT_EQ(header.request_line.http_version.minor(), 1);
	EXPECT_EQ(std::string_view{res.ptr}, "Some additional data");
}
