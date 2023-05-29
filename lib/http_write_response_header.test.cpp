//@	{"target":{"name":"http_write_response_header.test"}}

#include "./http_write_response_header.hpp"

#include <testfwk/testfwk.hpp>
#include <functional>

namespace
{
	struct data_sink
	{
		std::reference_wrapper<std::string> m_output_buffer;
		west::io::operation_result res{west::io::operation_result::completed};
		size_t max_length{4096};

		auto write(std::span<char const> buffer)
		{
			auto const bytes_to_write = std::min(std::min(std::size(buffer), max_length), static_cast<size_t>(13));
			std::copy_n(std::begin(buffer), bytes_to_write, std::back_inserter(m_output_buffer.get()));
			max_length -= bytes_to_write;
			return west::io::write_result{
				bytes_to_write,
				res
			};
		}

	};

	struct request_handler
	{};
}

TESTCASE(west_http_write_response_header_write_completed)
{
	std::array<char, 4096> buffer;
	west::io_adapter::buffer_span buff_span{buffer};

	west::http::response_header header;
	header.status_line.http_version = west::http::version{1, 1};
	header.status_line.status_code = west::http::status::ok;
	header.status_line.reason_phrase = "Foobar";
	header.fields.append("Content-Length", "143");

	west::http::write_response_header writer{header};
	std::string output_buffer;
	west::http::session session{data_sink{output_buffer},
		request_handler{},
		west::http::request_info{},
		west::http::response_header{}
	};

	while(true)
	{
		auto res = writer(buff_span, session);
		if(res.status == west::http::session_state_status::more_data_needed)
		{ EXPECT_EQ(output_buffer.empty(), true); }
		else
		if(res.status == west::http::session_state_status::completed)
		{
			std::string_view expected_output{
"HTTP/1.1 200 Foobar\r\n"
"Content-Length: 143\r\n"
"\r\n"
			};

			EXPECT_EQ(output_buffer, expected_output);
			EXPECT_EQ(std::size(buff_span.span_to_read()), 0);
			break;
		}
		else
		{
			printf("Unexpected io operation result %u\n", static_cast<uint32_t>(res.status));
			fflush(stdout);
			abort();
		}
	}

	fflush(stdout);
}

TESTCASE(west_http_write_response_header_connection_closed)
{
	std::array<char, 4096> buffer;
	west::io_adapter::buffer_span buff_span{buffer};

	west::http::response_header header;
	header.status_line.http_version = west::http::version{1, 1};
	header.status_line.status_code = west::http::status::ok;
	header.status_line.reason_phrase = "Foobar";
	header.fields.append("Content-Length", "143");

	west::http::write_response_header writer{header};
	std::string output_buffer;
	west::http::session session{data_sink{output_buffer, west::io::operation_result::completed, 17},
		request_handler{},
		west::http::request_info{},
		west::http::response_header{}
	};

	auto res = writer(buff_span, session);
	EXPECT_EQ(res.status, west::http::session_state_status::connection_closed);
}