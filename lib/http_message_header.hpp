#ifndef WEST_HTTP_MESSAGE_HEADER_HPP
#define WEST_HTTP_MESSAGE_HEADER_HPP

#include <string>
#include <map>
#include <cstdint>

namespace west::http
{
	class version
	{
	public:
		constexpr version():version{0, 0} {}

		constexpr explicit version(uint32_t major, uint32_t minor):
		m_value{(static_cast<uint64_t>(major) << 32llu) | minor}
		{ }

		constexpr auto operator<=>(version const&) const = default;

		constexpr uint32_t minor() const
		{ return m_value & 0xffff'ffff; }

		constexpr uint32_t major() const
		{ return static_cast<uint32_t>(m_value >> 32llu); }

		constexpr version& minor(uint32_t val)
		{
			m_value = (m_value & 0xffff'ffff'0000'0000) | val;
			return *this;
		}

		constexpr version& major(uint32_t val)
		{
			m_value = (m_value & 0xffff'ffff) | (static_cast<uint64_t>(val) << 32llu);
			return *this;
		}

	private:
		uint64_t m_value;
	};

	enum class status
	{
		ok = 200,
		created = 201,
		accepted = 202,
		non_authoritative_information = 203,
		no_content = 204,
		reset_content = 205,
		partial_content = 206,

		bad_request = 400,
		unauthorized = 401,
		payment_required = 402,
		forbidden = 403,
		not_found = 404,
		method_not_allowed = 405,
		not_acceptable = 406,
		proxy_authentication_required = 407,
		request_timeout = 408,
		conflict = 409,
		gone = 410,
		length_required = 411,
		precondition_failed = 412,
		request_entity_too_large = 413,
		request_uri_too_long = 414,
		unsupported_media_type = 415,
		requested_range_not_satisfiable = 416,
		expectation_failed = 417,
		i_am_a_teapot = 418,
		misdirected_request = 421,
		unprocessable_content = 422,
		failed_dependency = 423,
		too_early = 425,
		upgrade_required = 426,
		precondition_required = 428,
		too_many_requests = 429,
		request_header_fields_too_large = 431,
		unavailable_for_legal_resons = 451,

		internal_server_error = 500,
		not_implemented = 501,
		bad_gateway = 502,
		service_unavailable = 503,
		gateway_timeout = 504,
		http_version_not_supported = 505
	};

	class field_map
	{
	public:
		field_map& append(std::string&& field_name, std::string_view value)
		{
			auto ip = m_fields.emplace(std::move(field_name), value);
			if(ip.second)
			{ return *this; }

			auto& val = ip.first->second;
			if(value.size() != 0)
			{ val.append(", ").append(value); }
			return *this;
		}

		auto begin() const
		{ return std::begin(m_fields); }

		auto end() const
		{ return std::end(m_fields); }

		auto find(std::string_view field_name) const
		{ return m_fields.find(field_name); }

	private:
		std::map<std::string, std::string, std::less<>> m_fields;
	};

	struct request_line
	{
		std::string method;
		std::string request_target;
		version http_version;
	};

	struct request_header
	{
		struct request_line request_line;
		field_map fields;
	};

	struct status_line
	{
		version http_version;
		status status_code;
		std::string reason_phrase;
	};

	struct response_header
	{
		struct status_line status_line;
		field_map fields;
	};
}

#endif