//@	{"target":{"name":"http_read_request_body.test"}}"

#include "./http_read_request_body.hpp"

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

			auto bytes_to_read =std::min(std::min(std::size(output_buffer), remaining), static_cast<size_t>(23));
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

		auto get_data() const { return m_buffer; }

	private:
		std::string_view m_buffer;
		char const* m_ptr;
		std::minstd_rand m_rng;
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

	struct result
	{
		char const* ptr;
		test_result ec;
	};


	template<test_result Result>
	struct request_handler
	{
		auto process_request_content(std::span<char const> buffer)
		{
			data_processed.insert(std::end(data_processed), std::begin(buffer), std::end(buffer));

			return result{
				.ptr = std::data(buffer) + std::size(buffer),
				.ec = Result
			};
		}

		auto finalize_state(west::http::read_request_body_tag) const
		{
			return west::http::session_state_response{
				.status = west::http::session_state_status::completed,
				.http_status = west::http::status::accepted,
				.error_message = west::make_unique_cstr("Hej")
			};
		}

		std::string data_processed;
	};
}

TESTCASE(http_read_request_body_read_all_data)
{
	std::array<char, 4096> buffer{};
	west::io::buffer_view buffer_view{buffer};

	data_source src{std::string_view{"Aenean at placerat tortor. Proin tristique sit amet elit vitae "
"tristique. Vivamus pellentesque  imperdiet iaculis. Phasellus vel elit convallis, vestibulum diam "
"eu, sollicitudin risus. Orci  varius natoque penatibus et magnis dis parturient montes, nascetur "
"ridiculus mus. In auctor iaculis augue, sit amet dapibus massa porta non. Vestibulum porttitor "
"blandit libero, quis molestie velit finibus sed. Ut id ante dictum, dignissim elit id, rhoncus "
"augue. Pellentesque vel congue neque. Orci varius natoque penatibus et magnis dis parturient "
"montes, nascetur ridiculus mus. Vivamus in augue at nisl luctus posuere sed in dui. Nunc elit "
"ipsum, ultricies at nulla eget, luctus viverra purus. Aenean dignissim hendrerit ex, ut posuere "
"nunc semper at. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac "
"turpis egestas. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec quis efficitur "
"enim."}};

	west::http::session session{src, request_handler<test_result::ok>{}, west::http::request_header{}};
	west::http::read_request_body reader{src.get_data().size()};

	while(true)
	{
		auto res = reader(buffer_view, session);
		if(res.status != west::http::session_state_status::more_data_needed)
		{
			EXPECT_EQ(res.status, west::http::session_state_status::completed);
			EXPECT_EQ(res.http_status, west::http::status::accepted);
			EXPECT_EQ(res.error_message.get(), std::string_view{"Hej"});
			EXPECT_EQ(session.request_handler.data_processed, src.get_data());
			break;
		}
	}
}