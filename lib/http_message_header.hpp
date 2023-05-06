#ifndef WEST_HTTP_MESSAGE_HEADER_HPP
#define WEST_HTTP_MESSAGE_HEADER_HPP

#include <string>
#include <cstdint>

namespace west::http
{
	class version
	{
	public:
		constexpr version():version{1, 1} {}

		constexpr explicit version(uint32_t major, uint32_t minor):
		m_value{(static_cast<uint64_t>(major) << 32llu) | minor}
		{ }

		constexpr auto operator<=>(version const&) const = default;

		constexpr uint32_t minor() const
		{ return m_value & 0xffff'ffff; }

		constexpr uint32_t major() const
		{ return static_cast<uint32_t>(m_value >> 32llu); }

	private:
		uint64_t m_value;
	};

	struct request_line
	{
		std::string method;
		std::string request_target;
		version http_version;
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

	struct status_line
	{
		version http_version;
		status status_code;
		std::string reason_phrase;
	};

}

#endif