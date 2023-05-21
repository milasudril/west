//@	{"target":{"name":"http_read_request_body.test"}}"

#include "./http_read_request_body.hpp"

#include "stubs/data_source.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	enum class test_result{completed, is_blocked, error};

	constexpr bool should_return(test_result res)
	{ return res != test_result::completed; }

	constexpr char const* to_string(test_result res)
	{
		switch(res)
		{
			case test_result::completed:
				return "Completed";
			case test_result::is_blocked:
				return "Is blocked";
			case test_result::error:
				return "Error";
		}
		__builtin_unreachable();
	}

	constexpr bool can_continue(test_result res)
	{ return res == test_result::is_blocked; }

	struct result
	{
		size_t bytes_written;
		test_result ec;
	};


	template<test_result Result, bool RejInFinalize = false>
	struct request_handler
	{
		auto process_request_content(std::span<char const> buffer)
		{
			data_processed.insert(std::end(data_processed), std::begin(buffer), std::end(buffer));

			return result{
				.bytes_written = std::size(buffer),
				.ec = Result
			};
		}

		auto finalize_state(west::http::field_map& resp_header_fields) const
		{
			resp_header_fields.append("Kaka", "Foobar");

			return west::http::session_state_response{
				.status = west::http::session_state_status::completed,
				.http_status = RejInFinalize?
					west::http::status::i_am_a_teapot:
					west::http::status::accepted,
				.error_message = west::make_unique_cstr("Hej")
			};
		}

		std::string data_processed;
	};
}

TESTCASE(http_read_request_body_read_all_data)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{
"Aenean at placerat tortor. Proin tristique sit amet elit vitae tristique. Vivamus pellentesque "
"imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam eu, sollicitudin risus. Orci "
"varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In auctor "
"iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor blandit libero, quis "
"molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus augue. Pellentesque vel "
"congue neque. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
"mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit ipsum, ultricies at nulla eget,"
" luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere nunc semper at. Pellentesque "
" habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Lorem ipsum "
" dolor sit amet, consectetur adipiscing elit. Donec quis efficitur enim.Some additional data."}};

	west::http::session session{src,
		request_handler<test_result::completed>{},
		west::http::request_header{},
		west::http::response_header{}
	};
	auto const message_size = src.get_data().size() - 21;
	west::http::read_request_body reader{message_size};

	auto res = reader(buff_span, session);
	EXPECT_EQ(res.status, west::http::session_state_status::completed);
	EXPECT_EQ(res.http_status, west::http::status::accepted);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Hej"});
	EXPECT_EQ(session.response_header.status_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.response_header.status_line.reason_phrase, "Accepted");
	EXPECT_EQ(session.response_header.fields.find("Kaka")->second, "Foobar");
	EXPECT_EQ(session.request_handler.data_processed, src.get_data().substr(0, message_size));
	EXPECT_EQ(buff_span.span_to_read()[0], 'S');
}


TESTCASE(http_read_request_body_read_early_eof)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{
"Aenean at placerat tortor. Proin tristique sit amet elit vitae tristique. Vivamus pellentesque "
"imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam eu, sollicitudin risus. Orci "
"varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In auctor "
"iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor blandit libero, quis "
"molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus augue. Pellentesque vel "
"congue neque. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
"mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit ipsum, ultricies at nulla eget,"
" luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere nunc semper at. Pellentesque "
" habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Lorem ipsum "
" dolor sit amet, consectetur adipiscing elit. Donec quis efficitur enim."}};

	west::http::session session{src,
		request_handler<test_result::completed>{},
		west::http::request_header{},
		west::http::response_header{}
	};
	auto const message_size = src.get_data().size() + 23;
	west::http::read_request_body reader{message_size};

	auto res = reader(buff_span, session);
	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Client claims there is more data to read"});
}

TESTCASE(http_read_request_body_read_req_handler_fails_to_read)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{
"Aenean at placerat tortor. Proin tristique sit amet elit vitae tristique. Vivamus pellentesque "
"imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam eu, sollicitudin risus. Orci "
"varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In auctor "
"iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor blandit libero, quis "
"molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus augue. Pellentesque vel "
"congue neque. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
"mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit ipsum, ultricies at nulla eget,"
" luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere nunc semper at. Pellentesque "
" habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Lorem ipsum "
" dolor sit amet, consectetur adipiscing elit. Donec quis efficitur enim.Some additional data."}};

	west::http::session session{src,
		request_handler<test_result::error>{},
		west::http::request_header{},
		west::http::response_header{}
	};
	auto const message_size = src.get_data().size() - 21;
	west::http::read_request_body reader{message_size};

	auto res = reader(buff_span, session);
	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::bad_request);
	EXPECT_EQ(res.error_message.get(), std::string_view{"Error"});
}

TESTCASE(http_read_request_body_read_req_handler_blocks_when_reading)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{
"Aenean at placerat tortor. Proin tristique sit amet elit vitae tristique. Vivamus pellentesque "
"imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam eu, sollicitudin risus. Orci "
"varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In auctor "
"iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor blandit libero, quis "
"molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus augue. Pellentesque vel "
"congue neque. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
"mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit ipsum, ultricies at nulla eget,"
" luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere nunc semper at. Pellentesque "
" habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Lorem ipsum "
" dolor sit amet, consectetur adipiscing elit. Donec quis efficitur enim.Some additional data."}};

	west::http::session session{src,
		request_handler<test_result::is_blocked>{},
		west::http::request_header{},
		west::http::response_header{}
	};
	auto const message_size = src.get_data().size() - 21;
	west::http::read_request_body reader{message_size};

	auto res = reader(buff_span, session);
	EXPECT_EQ(res.status, west::http::session_state_status::more_data_needed);
	EXPECT_EQ(res.http_status, west::http::status::ok);
	EXPECT_EQ(res.error_message.get(), nullptr);
}

TESTCASE(http_read_request_body_read_req_handler_rej_in_finalize)
{
	std::array<char, 4096> buffer{};
	west::io_adapter::buffer_span buff_span{buffer};

	west::stubs::data_source src{std::string_view{
"Aenean at placerat tortor. Proin tristique sit amet elit vitae tristique. Vivamus pellentesque "
"imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam eu, sollicitudin risus. Orci "
"varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In auctor "
"iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor blandit libero, quis "
"molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus augue. Pellentesque vel "
"congue neque. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
"mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit ipsum, ultricies at nulla eget,"
" luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere nunc semper at. Pellentesque "
" habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Lorem ipsum "
" dolor sit amet, consectetur adipiscing elit. Donec quis efficitur enim.Some additional data."}};

	west::http::session session{src,
		request_handler<test_result::completed, true>{},
		west::http::request_header{},
		west::http::response_header{}
	};
	auto const message_size = src.get_data().size() - 21;
	west::http::read_request_body reader{message_size};

	auto res = reader(buff_span, session);
	EXPECT_EQ(res.status, west::http::session_state_status::client_error_detected);
	EXPECT_EQ(res.http_status, west::http::status::i_am_a_teapot);
	EXPECT_EQ(session.response_header.status_line.http_version, (west::http::version{1, 1}));
	EXPECT_EQ(session.response_header.status_line.reason_phrase, "I am a teapot");
	EXPECT_EQ(res.error_message.get(), std::string_view{"Hej"});
}
