//@	{"target":{"name":"http_message_header.test"}}

#include "./http_message_header.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(west_http_version)
{
	west::http::version ver{};
	EXPECT_EQ(ver.major(), 0);
	EXPECT_EQ(ver.minor(), 0);

	ver.major(123);
	EXPECT_EQ(ver.major(), 123);
	EXPECT_EQ(ver.minor(), 0);

	ver.minor(456);
	EXPECT_EQ(ver.major(), 123);
	EXPECT_EQ(ver.minor(), 456);

	ver.major(45).minor(7);
	EXPECT_EQ(ver.major(), 45);
	EXPECT_EQ(ver.minor(), 7);

	EXPECT_EQ(ver, (west::http::version{45, 7}));

	auto str = to_string(ver);
	EXPECT_EQ(str, "45.7");
}

TESTCASE(west_http_status_to_string)
{
	EXPECT_EQ(to_string(west::http::status::ok), std::string_view{"Ok"});
	EXPECT_EQ(to_string(west::http::status::created), std::string_view{"Created"});
	EXPECT_EQ(to_string(west::http::status::accepted), std::string_view{"Accepted"});
	EXPECT_EQ(to_string(west::http::status::non_authoritative_information), std::string_view{"Non-authoritative information"});
	EXPECT_EQ(to_string(west::http::status::no_content), std::string_view{"No content"});
	EXPECT_EQ(to_string(west::http::status::reset_content), std::string_view{"Reset content"});
	EXPECT_EQ(to_string(west::http::status::partial_content), std::string_view{"Partial content"});

	EXPECT_EQ(to_string(west::http::status::bad_request), std::string_view{"Bad request"});
	EXPECT_EQ(to_string(west::http::status::unauthorized), std::string_view{"Unauthorized"});
	EXPECT_EQ(to_string(west::http::status::payment_required), std::string_view{"Payment required"});
	EXPECT_EQ(to_string(west::http::status::forbidden), std::string_view{"Forbidden"});
	EXPECT_EQ(to_string(west::http::status::not_found), std::string_view{"Not found"});
	EXPECT_EQ(to_string(west::http::status::method_not_allowed), std::string_view{"Method not allowed"});
	EXPECT_EQ(to_string(west::http::status::not_acceptable), std::string_view{"Not acceptable"});
	EXPECT_EQ(to_string(west::http::status::proxy_authentication_required), std::string_view{"Proxy authentication required"});
	EXPECT_EQ(to_string(west::http::status::request_timeout), std::string_view{"Request timeout"});
	EXPECT_EQ(to_string(west::http::status::conflict), std::string_view{"Conflict"});
	EXPECT_EQ(to_string(west::http::status::gone), std::string_view{"Gone"});
	EXPECT_EQ(to_string(west::http::status::length_required), std::string_view{"Length required"});
	EXPECT_EQ(to_string(west::http::status::precondition_failed), std::string_view{"Precondition failed"});
	EXPECT_EQ(to_string(west::http::status::request_entity_too_large), std::string_view{"Request entity too large"});
	EXPECT_EQ(to_string(west::http::status::request_uri_too_long), std::string_view{"Request uri too long"});
	EXPECT_EQ(to_string(west::http::status::unsupported_media_type), std::string_view{"Unsupported media type"});
	EXPECT_EQ(to_string(west::http::status::requested_range_not_satisfiable), std::string_view{"Requested range not satisfiable"});
	EXPECT_EQ(to_string(west::http::status::expectation_failed), std::string_view{"Expectation failed"});
	EXPECT_EQ(to_string(west::http::status::i_am_a_teapot), std::string_view{"I am a teapot"});
	EXPECT_EQ(to_string(west::http::status::misdirected_request), std::string_view{"Misdirected request"});
	EXPECT_EQ(to_string(west::http::status::unprocessable_content), std::string_view{"Unprocessable content"});
	EXPECT_EQ(to_string(west::http::status::failed_dependency), std::string_view{"Failed dependency"});
	EXPECT_EQ(to_string(west::http::status::too_early), std::string_view{"Too early"});
	EXPECT_EQ(to_string(west::http::status::upgrade_required), std::string_view{"Upgrade required"});
	EXPECT_EQ(to_string(west::http::status::precondition_required), std::string_view{"Precondition required"});
	EXPECT_EQ(to_string(west::http::status::too_many_requests), std::string_view{"Too many requests"});
	EXPECT_EQ(to_string(west::http::status::request_header_fields_too_large), std::string_view{"Request header fields too large"});
	EXPECT_EQ(to_string(west::http::status::unavailable_for_legal_reasons), std::string_view{"Unavailable for legal reasons"});

	EXPECT_EQ(to_string(west::http::status::internal_server_error), std::string_view{"Internal server error"});
	EXPECT_EQ(to_string(west::http::status::not_implemented), std::string_view{"Not implemented"});
	EXPECT_EQ(to_string(west::http::status::bad_gateway), std::string_view{"Bad gateway"});
	EXPECT_EQ(to_string(west::http::status::service_unavailable), std::string_view{"Service unavailable"});
	EXPECT_EQ(to_string(west::http::status::gateway_timeout), std::string_view{"Gateway timeout"});
	EXPECT_EQ(to_string(west::http::status::http_version_not_supported), std::string_view{"Http version not supported"});
}

TESTCASE(west_http_is_delimiter)
{
	for(int k = 0; k != 256; ++k)
	{
		auto ch = static_cast<char>(k);
		if(ch == '"' || ch == '(' || ch == ')' || ch == ',' || ch == '/' || ch == ':'
			|| ch == ';' || ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '@'
			|| ch == '[' || ch == '\\' || ch == ']' || ch == '{' || ch == '}')
		{ EXPECT_EQ(west::http::is_delimiter(ch), true); }
		else
		{ EXPECT_EQ(west::http::is_delimiter(ch), false); }
	}
}

TESTCASE(west_http_is_not_printable)
{
	for(int k = 0; k != 256; ++k)
	{
		auto ch = static_cast<char>(k);
		if((ch>= '\0' && ch <=' ') || ch == 127)
		{ EXPECT_EQ(west::http::is_not_printable(ch), true); }
		else
		{ EXPECT_EQ(west::http::is_not_printable(ch), false); }
	}
}

TESTCASE(west_http_is_strict_whitespace)
{
	for(int k = 0; k != 256; ++k)
	{
		auto ch = static_cast<char>(k);
		if(ch == ' ' || ch == '\t')
		{ EXPECT_EQ(west::http::is_strict_whitespace(ch), true); }
		else
		{ EXPECT_EQ(west::http::is_strict_whitespace(ch), false); }
	}
}

TESTCASE(west_http_stricmp)
{
	EXPECT_EQ(west::http::stricmp("foobar", "FOOBAR"), 0);
	EXPECT_EQ(west::http::stricmp("FoObAr", "fOoBaR"), 0);
}

TESTCASE(west_http_is_token)
{
	EXPECT_EQ(west::http::is_token(""), false);
	EXPECT_EQ(west::http::is_token("some_text_ä"), false);
	EXPECT_EQ(west::http::is_token("some_text_\""), false);
	EXPECT_EQ(west::http::is_token("some_text "), false);
	EXPECT_EQ(west::http::is_token("some_text\x7f"), false);
	EXPECT_EQ(west::http::is_token("Hej"), true);
}

TESTCASE(west_http_contains_whitespace)
{
	EXPECT_EQ(west::http::contains_whitespace("\x7f"), true);
	EXPECT_EQ(west::http::contains_whitespace("this text contains whitespace"), true);
	EXPECT_EQ(west::http::contains_whitespace("this_text_does_not_contain_whitespace"), false);
}

TESTCASE(west_http_request_method_default)
{
	west::http::request_method req{};
	EXPECT_EQ(req.has_value(), false);
}

TESTCASE(west_http_request_method_create)
{
	{
		auto res = west::http::request_method::create("foobar");
		REQUIRE_EQ(res.has_value(), true);
		EXPECT_EQ(*res, "foobar");
		EXPECT_NE(*res, "Foobar");
	}

	{
		auto res = west::http::request_method::create("");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::request_method::create("foo bar");
		EXPECT_EQ(res.has_value(), false);
	}
}

TESTCASE(west_http_uri_default)
{
	west::http::uri uri{};
	EXPECT_EQ(uri.has_value(), false);
}

TESTCASE(west_http_uri_create)
{
	{
		auto res = west::http::uri::create("foobar/umläütß");
		REQUIRE_EQ(res.has_value(), true);
		EXPECT_EQ(*res, "foobar/umläütß");
		EXPECT_NE(*res, "Foobar/umläütß");
	}

	{
		auto res = west::http::uri::create("foo bar");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::uri::create("");
		EXPECT_EQ(res.has_value(), false);
	}
}

TESTCASE(west_http_field_name_create)
{
	{
		auto res = west::http::field_name::create("foobar");
		REQUIRE_EQ(res.has_value(), true);
		EXPECT_EQ(*res, "foobar");
		EXPECT_EQ(*res, "Foobar");
		EXPECT_EQ(res->value(), "foobar");
		EXPECT_NE(res->value(), "Foobar");
	}

	{
		auto res = west::http::field_name::create("");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::field_name::create("foo bar");
		EXPECT_EQ(res.has_value(), false);
	}
}

TESTCASE(west_http_field_value_create)
{
	{
		auto res = west::http::field_value::create("");
		REQUIRE_EQ(res.has_value(), true);
		EXPECT_EQ(*res, "");
	}

	{
		auto res = west::http::field_value::create(" foo");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::field_value::create("foo ");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::field_value::create(" foo ");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::field_value::create("a string\nwith junk");
		EXPECT_EQ(res.has_value(), false);
	}

	{
		auto res = west::http::field_value::create("a string with whitespace");
		REQUIRE_EQ(res.has_value(), true);
		EXPECT_EQ(*res, "a string with whitespace");
		EXPECT_NE(*res, "A string with whitespace");
		EXPECT_EQ(res->value(), "a string with whitespace");
	}
}