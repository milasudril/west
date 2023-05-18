//@	{"target":{"name":"http_request_processor.test"}}

#include "./http_request_processor.hpp"

#include <testfwk/testfwk.hpp>
#include <random>

namespace
{
	class data_source
	{
	public:
		explicit data_source(std::string_view buffer, std::string& output_buffer):
			m_buffer{buffer},
			m_ptr{std::begin(buffer)},
			m_output_buffer{output_buffer}
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

			auto const bytes_to_read = std::min(std::min(std::size(output_buffer), remaining), static_cast<size_t>(13));
			std::copy_n(m_ptr, remaining, std::begin(output_buffer));
			m_ptr += bytes_to_read;
			return west::io::read_result{
				.bytes_read = bytes_to_read,
				.ec = bytes_to_read == 0 ?
					west::io::operation_result::completed:
					west::io::operation_result::more_data_present
			};
		}

		auto write(std::span<char const> data)
		{
			std::bernoulli_distribution io_should_block{0.25};
			if(io_should_block(m_rng))
			{
				return west::io::write_result{
					.bytes_written = 0,
					.ec = west::io::operation_result::operation_would_block
				};
			}

			auto const bytes_to_write = std::min(std::size(data), static_cast<size_t>(13));
			m_output_buffer.get().insert(std::size(m_output_buffer.get()), std::data(data), bytes_to_write);

			return west::io::write_result{
				.bytes_written = bytes_to_write,
				.ec = west::io::operation_result::more_data_present
			};
		}

		void stop_reading()
		{ m_ptr = std::end(m_buffer); }

		char const* get_pointer() const { return m_ptr; }

	private:
		std::string_view m_buffer;
		char const* m_ptr;
		std::minstd_rand m_rng;

		std::reference_wrapper<std::string> m_output_buffer;
	};

	enum class test_result{ok, failure};

	constexpr bool is_error_indicator(test_result res)
	{
		return res != test_result::ok;
	}

	constexpr char const* to_string(test_result res)
	{
		switch(res)
		{
			case test_result::ok:
				return "Ok";
			case test_result::failure:
				return "Failure";
		}
		__builtin_unreachable();
	}

	struct content_proc_result
	{
		char const* ptr;
		test_result ec;
	};

	struct content_read_result
	{
		char* ptr;
		test_result ec;
	};

	struct request_handler
	{
		auto finalize_state(west::http::request_header const&) const
		{
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = west::make_unique_cstr("This string comes from the test case");
			return validation_result;
		}

		template<class T>
		auto finalize_state(T) const
		{
			return west::http::finalize_state_result{};
		}

		auto process_request_content(std::span<char const> buffer)
		{
			request_body.insert(std::end(request_body), std::begin(buffer), std::end(buffer));
			return content_proc_result{
				.ptr = std::data(buffer) + std::size(buffer),
				.ec = test_result::ok
			};
		}

		auto read_response_content(std::span<char>)
		{

			return content_read_result{};
		}

		std::string request_body;
	};
}

template<size_t N>
struct foo{};

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
"Content-Length: 469\r\n"
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
"purus id elit. Nunc vel mollis tellus. Pellentesque lacinia mollis turpis tempor mattis.Some more data."};

	std::string output_buffer;
	west::http::request_processor proc{data_source{serialized_header, output_buffer}, request_handler{}};

	while(true)
	{
		auto const res = proc.socket_is_ready();
		if(res != west::http::request_processor_status::more_data_needed)
		{
			printf("(%s)\n", proc.get_request_handler().request_body.c_str());
			printf("(%s)\n", output_buffer.c_str());
			EXPECT_EQ(res, west::http::request_processor_status::completed);
			break;
		}
	}
}