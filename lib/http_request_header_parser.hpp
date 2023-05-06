#ifndef WEST_HTTP_REQUEST_PARSER_HPP
#define WEST_HTTP_REQUEST_PARSER_HPP

#include "./http_message_header.hpp"

namespace west::http
{
	class request_header_parser
	{
	public:
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

			skip_lf_after_cr,
			skip_cr_after_lf
		};
	};
}

#endif