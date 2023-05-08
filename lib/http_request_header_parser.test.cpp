//@	{"target":{"name":"http_request_header_parser.test"}}

#include "./http_request_header_parser.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_request_header_parser_parse_complete_header)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/111.0\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;\r\n"
"\t q=0.9,image/avif,image/webp,*/*;\r\n"
" \r\n"
" \tq=0.8\r\n"
"Accept-Language: sv-SE,sv;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"DNT: 1\r\n"
"Connection: keep-alive\r\n"
"Key-without-value-1:   \r\n"
"Key-without-value-2:\r\n"
"Key-with-value-between-whitespace:  foo   \r\n"
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
	EXPECT_EQ(header.fields.find("host")->second, "localhost:8000");
	EXPECT_EQ(header.fields.find("user-agent")->second,
		"Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/111.0");
	EXPECT_EQ(header.fields.find("accept")->second,
		"text/html,application/xhtml+xml,application/xml; "
		"q=0.9,image/avif,image/webp,*/*; "
		"q=0.8");
	EXPECT_EQ(header.fields.find("accept-language")->second,
		"sv-SE,sv;q=0.8,en-US;q=0.5,en;q=0.3");
	EXPECT_EQ(header.fields.find("accept-encoding")->second, "gzip, deflate, br");
	EXPECT_EQ(header.fields.find("dnt")->second, "1");
	EXPECT_EQ(header.fields.find("connection")->second, "keep-alive");
	EXPECT_EQ(header.fields.find("key-without-value-1")->second, "");
	EXPECT_EQ(header.fields.find("key-without-value-2")->second, "");
	EXPECT_EQ(header.fields.find("upgrade-insecure-requests")->second, "1");
	EXPECT_EQ(header.fields.find("sec-fetch-dest")->second, "document");
	EXPECT_EQ(header.fields.find("sec-fetch-mode")->second, "navigate");
	EXPECT_EQ(header.fields.find("sec-fetch-site")->second, "none");
	EXPECT_EQ(header.fields.find("sec-fetch-user")->second, "?1");
	EXPECT_EQ(header.fields.find("Key-with-value-between-whitespace")->second, "foo");
}

TESTCASE(west_http_request_header_parser_parse_no_fields)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
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
	EXPECT_EQ(header.fields.empty(), true);
}

TESTCASE(west_http_request_header_parser_parse_last_field_has_no_value)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"a-field:\r\n"
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
	EXPECT_EQ(header.fields.find("a-field")->second, "");
}