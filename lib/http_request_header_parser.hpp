#ifndef WEST_HTTP_REQUEST_HEADER_PARSER_HPP
#define WEST_HTTP_REQUEST_HEADER_PARSER_HPP

#include "./http_message_header.hpp"

#include <functional>
#include <optional>
#include <charconv>
#include <cassert>
#include <ranges>

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
		bad_request_method,
		bad_request_target,
		wrong_protocol,
		bad_protocol_version,
		expected_linefeed,
		bad_field_name,
		bad_field_value
	};

	constexpr char const* to_string(req_header_parser_error_code ec)
	{
		switch(ec)
		{
			case req_header_parser_error_code::completed:
				return "Completed";

			case req_header_parser_error_code::more_data_needed:
				return "More data needed";

			case req_header_parser_error_code::bad_request_method:
				return "Bad request method";

 			case req_header_parser_error_code::bad_request_target:
				return "Bad request target";

			case req_header_parser_error_code::wrong_protocol:
				return "Wrong protocol";

			case req_header_parser_error_code::bad_protocol_version:
				return "Bad protocol version";

			case req_header_parser_error_code::expected_linefeed:
				return "Expected linefeed";

			case req_header_parser_error_code::bad_field_name:
				return "Bad field name";

				case req_header_parser_error_code::bad_field_value:
				return "Bad field value";
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
		if(res.ptr != std::end(val) || res.ec == std::errc::result_out_of_range)
		{ return std::nullopt; }

		return ret;
	}

	inline void rtrim(std::string& str)
	{
		auto i = std::ranges::find_if_not(std::ranges::reverse_view{str}, [](auto val){
			return is_strict_whitespace(val);
		});

		if(i.base() != std::end(str))
		{ str.erase(i.base(), std::end(str)); }
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
			expect_linefeed_and_return
		};

		state m_current_state;
		state m_state_after_newline;
		std::string m_buffer;
		std::string m_current_field_name;
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
		++ptr;

		switch(m_current_state)
		{
			case state::req_line_read_method:
				switch(ch_in)
				{
					case ' ':
						if(auto method = request_method::create(std::move(m_buffer)); method.has_value())
						{
							m_current_state = state::req_line_read_req_target;
							m_req_header.get().request_line.method = std::move(*method);
							m_buffer.clear();
						}
						else
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_request_method}; }
						break;

					default:
						m_buffer += ch_in;
				}
				break;

			case state::req_line_read_req_target:
				switch(ch_in)
				{
					case ' ':
						if(auto req_target = uri::create(std::move(m_buffer)); req_target.has_value())
						{
							m_current_state = state::req_line_read_protocol_name;
							m_req_header.get().request_line.request_target = std::move(*req_target);
							m_buffer.clear();
						}
						else
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_request_target}; }
						break;

					default:
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
						m_buffer += ch_in;
				}
				break;

			case state::req_line_read_protocol_version_minor:
				switch(ch_in)
				{
					case '\r':
						if(auto val = to_number<uint32_t>(m_buffer); val.has_value())
						{
							m_req_header.get().request_line.http_version.minor(*val);
							m_state_after_newline = state::fields_terminate_at_no_field_name;
							m_buffer.clear();
							m_current_state = state::expect_linefeed;
						}
						else
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_protocol_version}; }
						break;

					default:
						m_buffer += ch_in;
				}
				break;

			case state::expect_linefeed:
				if(ch_in != '\n')
				{ return req_header_parse_result{ptr, req_header_parser_error_code::expected_linefeed}; }
				m_current_state = m_state_after_newline;
				break;

			case state::fields_terminate_at_no_field_name:
				switch(ch_in)
				{
					case '\r':
						m_current_state = state::expect_linefeed_and_return;
						break;

					default:
						m_buffer += ch_in;
						m_current_state = state::fields_read_name;
				}
				break;

			case state::fields_read_name:
				switch(ch_in)
				{
					case ':':
						m_current_field_name = std::move(m_buffer);
						m_buffer.clear();
						m_current_state = state::fields_skip_ws_before_field_value;
						break;

					default:
						m_buffer += ch_in;
				}
				break;

			case state::fields_skip_ws_before_field_value:
				assert(m_buffer.empty());
				switch(ch_in)
				{
					case '\r':
					{
						auto key = field_name::create(std::move(m_current_field_name));
						if(!key.has_value())
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_field_name}; }
						m_current_field_name.clear();
						m_req_header.get().fields.append(std::move(*key), *field_value::create(""));

						m_state_after_newline = state::fields_terminate_at_no_field_name;
						m_current_state = state::expect_linefeed;
						break;
					}

					default:
						if(!is_strict_whitespace(ch_in))
						{
							m_buffer += ch_in;
							m_current_state = state::fields_read_value;
						}
				}
				break;

			case state::fields_read_value:
				switch(ch_in)
				{
					case '\r':
						m_state_after_newline = state::fields_check_continuation;
						m_current_state = state::expect_linefeed;
						break;

					default:
						m_buffer += ch_in;
				}
				break;

			case state::fields_check_continuation:
				switch(ch_in)
				{
					case '\t':
					case ' ':
						m_current_state = state::fields_skip_ws_after_newline;
						break;

					default:
					{
						auto key = field_name::create(std::move(m_current_field_name));
						if(!key.has_value())
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_field_name}; }
						m_current_field_name.clear();
						rtrim(m_buffer);
						auto value = field_value::create(std::move(m_buffer));
						if(!value.has_value())
						{ return req_header_parse_result{ptr, req_header_parser_error_code::bad_field_value}; }

						m_req_header.get().fields.append(std::move(*key), std::move(*value));
						m_buffer.clear();
						if(ch_in == '\r')
						{	m_current_state = state::expect_linefeed_and_return; }
						else
						{
							m_buffer += ch_in;
							m_current_state = state::fields_read_name;
						}
					}
				}
				break;

			case state::fields_skip_ws_after_newline:
				switch(ch_in)
				{
					case '\r':
						m_state_after_newline = state::fields_check_continuation;
						m_current_state = state::expect_linefeed;
						break;

					default:
						if(ch_in != '\t' && ch_in != ' ')
						{
							m_buffer += ' ';
							m_buffer += ch_in;
							m_current_state = state::fields_read_value;
						}
				}
				break;

			case state::expect_linefeed_and_return:
				if(ch_in == '\n')
				{ return req_header_parse_result{ptr, req_header_parser_error_code::completed}; }
				else
				{ return req_header_parse_result{ptr, req_header_parser_error_code::expected_linefeed}; }
		}
	}
}

#endif