#ifndef WEST_HTTP_REQUEST_PARSER_HPP
#define WEST_HTTP_REQUEST_PARSER_HPP

#include "./http_message_header.hpp"

#include <functional>

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
	};

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
			req_line_read_target,
			req_line_read_protocol_name,
			req_line_read_version_major,
			req_line_read_version_minor,

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
		++ptr;
	}
}

#endif