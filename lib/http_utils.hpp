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

	inline std::string encode_uri_component(std::string_view str)
	{
		std::string ret;
		ret.reserve(std::size(str));
		for(auto const item : str)
		{
			if((item >= 'A' && item <= 'Z') ||
				(item >= 'a' && item <= 'z') ||
				(item >= '0' && item <= '9') ||
				item == '-' || item == '_' ||
				item == '!' || item == '~' ||
				item == '*' || item == '\'' ||
				item == '(' || item == ')' || item == '.')
			{
				ret.push_back(item);
			}
			else
			{
				auto const msb = (item & 0xf0) >> 4;
				auto const lsb = (item & 0x0f);

				auto to_hex_digit = [](auto x) {
					return (x < 10) ? x + '0' : (x - 10) + 'A';
				};

				ret.push_back('%');
				ret.push_back(to_hex_digit(msb));
				ret.push_back(to_hex_digit(lsb));
			}
		}

		return ret;
	}

	inline std::string decode_uri_component(std::string_view str)
	{
		std::string ret;
		ret.reserve(std::size(str));
		enum class state{normal, escape_1, escape_2};
		auto current_state = state::normal;

		char decoded_value;

		for(auto const item : str)
		{
			auto from_hex_digit = [](auto x) {
				return x <= '9' ? x - '0' : (x - 'A') + 10;
			};

			switch(current_state)
			{
				case state::normal:
					switch(item)
					{
						case '%':
							current_state = state::escape_1;
							break;
						default:
							ret.push_back(item);
					}
					break;

				case state::escape_1:
					decoded_value = (from_hex_digit(item)<<4);
					current_state = state::escape_2;
					break;

				case state::escape_2:
					decoded_value |= from_hex_digit(item);
					ret.push_back(decoded_value);
					current_state = state::normal;
					break;
			};
		}

		return ret;
	}
}

#endif