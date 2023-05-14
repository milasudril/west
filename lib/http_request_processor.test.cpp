//@	{"target":{"name":"http_request_processor.test"}}

#include "./http_request_processor.hpp"

#include <testfwk/testfwk.hpp>
#include <random>

namespace
{
	class data_source
	{
	public:
		explicit data_source(std::string_view buffer):
			m_buffer{buffer},
			m_ptr{std::begin(buffer)}
		{}

		auto read(std::span<char> output_buffer)
		{
			auto const remaining = std::size(m_buffer) - (m_ptr - std::begin(m_buffer));

			std::bernoulli_distribution io_should_block{0.25};
			if(remaining != 0 && io_should_block(m_rng))
			{
				return west::io::read_result{
					.bytes_read = 0,
					.ec = west::io::operation_result::operation_would_block
				};
			}

			auto bytes_to_read =std::min(std::min(std::size(output_buffer), remaining), static_cast<size_t>(13));
			std::copy_n(m_ptr, remaining, std::begin(output_buffer));
			m_ptr += bytes_to_read;
			return west::io::read_result{
				.bytes_read = bytes_to_read,
				.ec = bytes_to_read == 0 ?
					west::io::operation_result::completed:
					west::io::operation_result::more_data_present
			};
		}

		auto write(std::span<char const>)
		{
			return west::io::write_result{
				.bytes_written = 0,
				.ec = west::io::operation_result::error
			};
		}

		void stop_reading()
		{ m_ptr = std::end(m_buffer); }

		char const* get_pointer() const { return m_ptr; }

	private:
		std::string_view m_buffer;
		char const* m_ptr;
		size_t m_callcount;
		std::minstd_rand m_rng;
	};

	struct request_handler
	{
		auto set_header(west::http::request_header&& header)
		{
			this->header = std::move(header);
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = west::make_unique_cstr("This string comes from the test case");
			return std::move(validation_result);
		}

		west::http::header_validation_result validation_result;
		west::http::request_header header;
	};
}

TESTCASE(west_http_request_processor_process_good_request)
{
	std::string_view serialized_header{"GET / HTTP/1.1\r\n"
"Host: localhost:8000\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/111.0\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;\r\n"
"\t q=0.9,image/avif,image/webp,*/*;\r\n"
" \r\n"
" \tq=0.8\r\n"
"Accept-Language: sv-SE,sv;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
"Accept-Encoding: gzip, deflate\r\n"
"Accept-Encoding: br\r\n"
"DNT: 1\r\n"
"Connection: keep-alive\r\n"
"Upgrade-Insecure-Requests: 1\r\n"
"Sec-Fetch-Dest: document\r\n"
"Sec-Fetch-Mode: navigate\r\n"
"Sec-Fetch-Site: none\r\n"
"Sec-Fetch-User: ?1\r\n"
"\r\n"
"Sed malesuada luctus velit nec consequat. Mauris congue aliquet tellus, tempus aliquam elit "
"sollicitudin quis. Donec justo massa, euismod a posuere in, finibus a tortor. Curabitur maximus "
"nibh vitae rhoncus commodo. Maecenas in velit laoreet ipsum tristique sodales nec eget mauris. "
"Morbi convallis, augue tristique congue facilisis, dui mauris cursus magna, sagittis rhoncus odio "
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis.\n"
"\n"
"Aenean at placerat tortor. Proin tristique sit amet elit vitae tristique. Vivamus pellentesque "
"imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam eu, sollicitudin risus. Orci "
"varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In auctor "
"iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor blandit libero, quis "
"molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus augue. Pellentesque vel "
"congue neque. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
"mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit ipsum, ultricies at nulla "
"eget, luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere nunc semper at. "
"Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. "
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec quis efficitur enim. "};

	west::http::request_processor proc{data_source{serialized_header}, request_handler{}};

	while(true)
	{
		auto const res = proc.socket_is_ready();
		if(res != west::http::request_processor_status::more_data_needed)
		{ break; }
	}
}