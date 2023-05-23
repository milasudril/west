#ifndef WEST_HTTP_RESPONSE_HEADER_SERIALIZER_HPP
#define WEST_HTTP_RESPONSE_HEADER_SERIALIZER_HPP

#include "./http_message_header.hpp"

#include <span>

namespace west::http
{
	struct resp_header_serializer_error_code
	{};

	constexpr bool is_error_indicator(resp_header_serializer_error_code)
	{ return false; }

	struct resp_header_serialize_result
	{
		char* ptr;
		resp_header_serializer_error_code ec;
	};

	class response_header_serializer
	{
	public:
		inline explicit response_header_serializer(response_header const& resp_header);

		resp_header_serialize_result serialize(std::span<char> output_buffer)
		{
			auto const n = std::min(std::size(output_buffer), std::size(m_range_to_write));
			std::copy_n(std::begin(m_range_to_write), n, std::begin(output_buffer));
			m_range_to_write = std::span{std::begin(m_range_to_write) + n, std::end(m_range_to_write)};

			return resp_header_serialize_result{
				std::data(output_buffer) + n,
				resp_header_serializer_error_code{}
			};
		}

	private:
		std::span<char const> m_range_to_write;
		std::string m_string_to_write;
	};

	response_header_serializer::response_header_serializer(response_header const& resp_header):
		m_string_to_write{"HTTP/"}
	{
		using status_int = std::underlying_type<status>::type;
		m_string_to_write.append(to_string(resp_header.status_line.http_version))
			.append(" ")
			.append(std::to_string(static_cast<status_int>(resp_header.status_line.status_code)))
			.append(" ")
			.append(resp_header.status_line.reason_phrase.empty() ?
				 to_string(resp_header.status_line.status_code)
				:resp_header.status_line.reason_phrase)
			.append("\r\n");

		for(auto const& item : resp_header.fields)
		{
			m_string_to_write.append(item.first.value())
				.append(": ")
				.append(item.second)
				.append("\r\n");
		}

		m_string_to_write.append("\r\n");

		m_range_to_write = std::span{std::begin(m_string_to_write), std::end(m_string_to_write)};
	}
}

#endif