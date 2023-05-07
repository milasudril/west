#ifndef WEST_HTTP_REQUEST_PARSER_HPP
#define WEST_HTTP_REQUEST_PARSER_HPP

#include "./http_message_header.hpp"

#include <functional>
#include <optional>
#include <charconv>

namespace west::http
{
	template<class T>
	concept req_header_parser_input_range = requires(T x)
	{
		{ std::begin(x) } -> std::bidirectional_iterator;
		{ std::end(x) } -> std::bidirectional_iterator;
		{ *std::begin(x) } -> std::convertible_to<char>;
	};

	enum class req_header_parser_error_code
	{
		completed,
		more_data_needed,
		wrong_protocol,
		bad_protocol_version
	};

	constexpr char const* to_string(req_header_parser_error_code ec)
	{
		switch(ec)
		{
			case req_header_parser_error_code::completed:
				return "Completed";

			case req_header_parser_error_code::more_data_needed:
				return "More data needed";

			case req_header_parser_error_code::wrong_protocol:
				return "Wrong protocol";

			case req_header_parser_error_code::bad_protocol_version:
				return "Bad bad_protocol_version";
		}

		__builtin_unreachable();
	}

	template<class T>
	requires(std::is_arithmetic_v<T>)
	inline std::optional<T> to_number(std::string_view val)
	{
		if(std::begin(val) == std::end(val))
		{ return std::nullopt; }

		T ret{};
		auto res = std::from_chars(std::begin(val), std::end(val), ret);
		if(res.ptr != std::end(val))
		{ return std::nullopt; }

		return ret;
	}


	template<class InputSeqIterator>
	struct req_header_parse_result
	{
		InputSeqIterator ptr;
		req_header_parser_error_code ec;
	};

	class request_header_parser
	{
	public:
		explicit request_header_parser(request_header& req_header):
			m_current_state{state::req_line_read_method},
			m_state_after_newline{state::req_line_read_method},
			m_req_header{req_header}
		{}

		template<req_header_parser_input_range InputSeq>
		auto parse(InputSeq input_seq);

	private:
		enum class state
		{
			req_line_read_method,
			req_line_read_req_target,
			req_line_read_protocol_name,
			req_line_read_protocol_version_major,
			req_line_read_protocol_version_minor,

			fields_terminate_at_no_field_name,
			fields_read_name,
			fields_skip_ws_before_field_value,
			fields_read_value,
			fields_check_continuation,
			fields_skip_ws_after_newline,

			expect_linefeed,
			expect_carriage_return
		};

		state m_current_state;
		state m_state_after_newline;
		std::string m_buffer;
		std::reference_wrapper<request_header> m_req_header;
	};
}

template<west::http::req_header_parser_input_range InputSeq>
auto west::http::request_header_parser::parse(InputSeq input_seq)
{
	auto ptr = std::begin(input_seq);
	while(true)
	{
		if(ptr == std::end(input_seq))
		{ return req_header_parse_result{ptr, req_header_parser_error_code::more_data_needed}; }

		auto ch_in = *ptr;
		(void)ch_in;
		++ptr;

		switch(m_current_state)
		{
			case state::req_line_read_method:
				switch(ch_in)
				{
					case ' ':
						m_current_state = state::req_line_read_req_target;
						m_req_header.get().request_line.method = std::move(m_buffer);
						m_buffer.clear();
						break;

					default:
						// FIXME: Check invalid char
						m_buffer += ch_in;
				}
				break;

			case state::req_line_read_req_target:
				switch(ch_in)
				{
					case ' ':
						m_current_state = state::req_line_read_protocol_name;
						m_req_header.get().request_line.request_target = std::move(m_buffer);
						m_buffer.clear();
						break;

					default:
						// FIXME: Check invalid char
						m_buffer += ch_in;
				}
				break;

			case state::req_line_read_protocol_name:
				switch(ch_in)
				{
					case '/':
						if(m_buffer != "HTTP")
						{ return req_header_parse_result{ptr, req_header_parser_error_code::wrong_protocol}; }

						m_buffer.clear();
						m_current_state = state::req_line_read_protocol_version_major;
						break;

					default:
						// FIXME: Check invalid char
						m_buffer += ch_in;
				}
				break;

			case state::req_line_read_protocol_version_major:
				switch(ch_in)
				{
					case '.':
						if(auto val = to_number<uint32_t>(m_buffer); val.has_value())
						{
							m_req_header.get().request_line.http_version.major(*val);
							m_current_state = state::req_line_read_protocol_version_minor;
							m_buffer.clear();
						}
						else
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_protocol_version}; }
						break;

					default:
						// FIXME: Check invalid char
						m_buffer += ch_in;
				}
				break;
			case state::req_line_read_protocol_version_minor:
				if(ch_in == '\n' || ch_in == '\r')
				{
					if(auto val = to_number<uint32_t>(m_buffer); val.has_value())
					{
						m_req_header.get().request_line.http_version.minor(*val);
						m_state_after_newline = state::fields_terminate_at_no_field_name;
						m_buffer.clear();
						if(ch_in == '\n')
						{ m_current_state = state::expect_carriage_return; }
						else
						{ m_current_state = state::expect_linefeed; }
					}
					else
					{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_protocol_version}; }
				}
				else
				{
					// FIXME: Check invalid char
					m_buffer += ch_in;
				}

			default:
				break;
		}
	}
}

#endif