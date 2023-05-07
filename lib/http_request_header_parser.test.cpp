//@	{"target":{"name":"http_request_header_parser.test"}}

#include "./http_request_header_parser.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_request_header_parser_parse_complete_header)
{
	std::string_view serialized_header{R"(GET / HTTP/1.1
Host: localhost:8000
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/111.0
Accept: text/html,application/xhtml+xml,application/xml;
	q=0.9,image/avif,image/webp,*/*;
	q=0.8
Accept-Language: sv-SE,sv;q=0.8,en-US;q=0.5,en;q=0.3
Accept-Encoding: gzip, deflate, br
DNT: 1
Connection: keep-alive
Upgrade-Insecure-Requests: 1
Sec-Fetch-Dest: document
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: none
Sec-Fetch-User: ?1

)"};

	west::http::request_header header{};
	west::http::request_header_parser parser{header};

	auto res = parser.parse(serialized_header);
	EXPECT_EQ(res.ec, west::http::req_header_parser_error_code::completed);
}