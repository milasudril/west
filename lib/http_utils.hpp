#ifndef WEST_HTTP_UTILS_HPP
#define WEST_HTTP_UTILS_HPP

#include "./http_message_header.hpp"

namespace west::http
{
	enum class cookie_string_parser_error_code{
		no_error,
		empty_key,
		invalid_key,
		duplicated_key,
		key_incomplete,
		single_space_expected
	};

	struct cookie_string_parse_result
	{
		char const* ptr;
		cookie_string_parser_error_code ec;
	};

	using cookie_store = std::map<std::string, std::string, std::less<>>;

	inline auto parse_cookie_string(std::string_view str, cookie_store& values)
	{
		auto ptr = std::data(str);
		auto bytes_left = std::size(str);
		std::string buffer;
		std::string key;
		enum class state{key, value, expect_one_whitespace};
		auto current_state = state::key;
		while(true)
		{
			if(bytes_left == 0)
			{
				if(key.empty())
				{ return cookie_string_parse_result{ptr, cookie_string_parser_error_code::key_incomplete}; }

				if(!values.insert(std::pair{std::move(key), std::move(buffer)}).second)
				{ return cookie_string_parse_result{ptr, cookie_string_parser_error_code::duplicated_key}; }

				return cookie_string_parse_result{ptr, cookie_string_parser_error_code::no_error};
			}

			auto const ch_in = *ptr;
			++ptr;
			--bytes_left;

			switch(current_state)
			{
				case state::key:
					switch(ch_in)
					{
						case '=':
							if(buffer.empty())
							{ return cookie_string_parse_result{ptr, cookie_string_parser_error_code::empty_key}; }

							if(!is_token(buffer))
							{ return cookie_string_parse_result{ptr, cookie_string_parser_error_code::invalid_key}; }

							key = std::move(buffer);
							buffer = std::string{};

							current_state = state::value;
							break;

						default:
							buffer += ch_in;
					}
					break;

				case state::value:
					switch(ch_in)
					{
						case ';':
							if(!values.insert(std::pair{std::move(key), std::move(buffer)}).second)
							{ return cookie_string_parse_result{ptr, cookie_string_parser_error_code::duplicated_key}; }

							key = std::string{};
							buffer = std::string{};

							current_state = state::expect_one_whitespace;
							break;

						default:
							buffer += ch_in;
					}
					break;

				case state::expect_one_whitespace:
					switch(ch_in)
					{
						case ' ':
							current_state = state::key;
							break;

						default:
							return cookie_string_parse_result{ptr, cookie_string_parser_error_code::single_space_expected};
					}
					break;
			}
		}
	}
}

#endif