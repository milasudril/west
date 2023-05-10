#ifndef WEST_HTTP_MESSAGE_HEADER_HPP
#define WEST_HTTP_MESSAGE_HEADER_HPP

#include <string>
#include <map>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <stdexcept>

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

	inline std::string to_string(version ver)
	{ return std::to_string(ver.major()).append(".").append(std::to_string(ver.minor())); }

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
		unavailable_for_legal_reasons = 451,

		internal_server_error = 500,
		not_implemented = 501,
		bad_gateway = 502,
		service_unavailable = 503,
		gateway_timeout = 504,
		http_version_not_supported = 505
	};

	constexpr char const* to_string(status val)
	{
		switch(val)
		{
			case status::ok:
				return "Ok";
			case status::created:
				return "Created";
			case status::accepted:
				return "Accepted";
			case status::non_authoritative_information:
				return "Non-authoritative information";
			case status::no_content:
				return "No content";
			case status::reset_content:
				return "Reset content";
			case status::partial_content:
				return "Partial content";

			case status::bad_request:
				return "Bad request";
			case status::unauthorized:
				return "Unauthorized";
			case status::payment_required:
				return "Payment required";
			case status::forbidden:
				return "Forbidden";
			case status::not_found:
				return "Not found";
			case status::method_not_allowed:
				return "Method not allowed";
			case status::not_acceptable:
				return "Not acceptable";
			case status::proxy_authentication_required:
				return "Proxy authentication required";
			case status::request_timeout:
				return "Request timeout";
			case status::conflict:
				return "Conflict";
			case status::gone:
				return "Gone";
			case status::length_required:
				return "Length required";
			case status::precondition_failed:
				return "Precondition failed";
			case status::request_entity_too_large:
				return "Request entity too large";
			case status::request_uri_too_long:
				return "Request uri too long";
			case status::unsupported_media_type:
				return "Unsupported media type";
			case status::requested_range_not_satisfiable:
				return "Requested range not satisfiable";
			case status::expectation_failed:
				return "Expectation failed";
			case status::i_am_a_teapot:
				return "I am a teapot";
			case status::misdirected_request:
				return "Misdirected request";
			case status::unprocessable_content:
				return "Unprocessable content";
			case status::failed_dependency:
				return "Failed dependency";
			case status::too_early:
				return "Too early";
			case status::upgrade_required:
				return "Upgrade required";
			case status::precondition_required:
				return "Precondition required";
			case status::too_many_requests:
				return "Too many requests";
			case status::request_header_fields_too_large:
				return "Request header fields too large";
			case status::unavailable_for_legal_reasons:
				return "Unavailable for legal reasons";

			case status::internal_server_error:
				return "Internal server error";
			case status::not_implemented:
				return "Not implemented";
			case status::bad_gateway:
				return "Bad gateway";
			case status::service_unavailable:
				return "Service unavailable";
			case status::gateway_timeout:
				return "Gateway timeout";
			case status::http_version_not_supported:
				return "Http version not supported";
		}
		__builtin_unreachable();
	}

	constexpr bool is_delimiter(char ch)
	{
		return ch == '"' || ch == '(' || ch == ')' || ch == ',' || ch == '/' || ch == ':'
			|| ch == ';' || ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '@'
			|| ch == '[' || ch == '\\' || ch == ']' || ch == '{' || ch == '}';
	}

	constexpr bool is_not_printable(char ch)
	{ return (ch >= '\0' && ch <= ' ')  || ch == 127; }

	constexpr bool is_strict_whitespace(char ch)
	{ return ch == ' ' || ch == '\t'; }

	inline auto stricmp(std::string_view a, std::string_view b)
	{
		return std::lexicographical_compare_three_way(std::begin(a), std::end(a),
			std::begin(b), std::end(b),
			[](unsigned char a, unsigned char b) {
				// NOTE: I assume that the local is "C". It does not make sense to set a local
				//       in a web server
				return std::toupper(a) <=> std::toupper(b);
			});
	}

	inline auto is_token(std::string_view str)
	{
		if(str.empty())
		{ return false; }

		if(std::ranges::any_of(str, [](auto val){
			return (val & 0x80) || is_delimiter(val) || is_not_printable(val);}))
		{return false;}

		return true;
	}

	inline auto contains_whitespace(std::string_view str)
	{
		if(std::ranges::any_of(str, [](auto val){ return is_not_printable(val); }))
		{return true;}

		return false;
	}

	class request_method
	{
	public:
		request_method() = default;

		bool has_value() const { return !m_value.empty(); }

		static std::optional<request_method> create(std::string&& str)
		{
			if(!is_token(str))
			{ return std::nullopt; }

			return request_method{std::move(str)};
		}

		bool operator==(request_method const&) const = default;
		bool operator!=(request_method const&) const = default;
		bool operator==(std::string_view other) const
		{ return m_value == other; }

		bool operator!=(std::string_view other) const
		{ return m_value != other; }

	private:
		explicit request_method(std::string&& string):m_value{std::move(string)}{}
		std::string m_value;
	};

	class uri
	{
	public:
		uri() = default;

		bool has_value() const { return !m_value.empty(); }

		static std::optional<uri> create(std::string&& str)
		{
			if(str.empty())
			{ return std::nullopt; }

			if(contains_whitespace(str))
			{ return std::nullopt; }

			return uri{std::move(str)};
		}

		bool operator==(uri const&) const = default;
		bool operator!=(uri const&) const = default;
		bool operator==(std::string_view other) const
		{ return m_value == other; }

		bool operator!=(std::string_view other) const
		{ return m_value != other; }

	private:
		explicit uri(std::string&& string):m_value{std::move(string)}{}
		std::string m_value;
	};

	class field_name
	{
	public:
		static std::optional<field_name> create(std::string&& str)
		{
			if(!is_token(str))
			{ return std::nullopt; }

			return field_name{std::move(str)};
		}

		auto operator<=>(field_name const& other) const
		{ return stricmp(m_value, other.m_value); }

		auto operator<=>(std::string_view val) const
		{ return stricmp(m_value, val); }


		bool operator==(field_name const&) const = default;
		bool operator!=(field_name const&) const = default;

		bool operator==(std::string_view val) const
		{ return (*this <=> val) == 0; }

		bool operator!=(std::string_view val) const
		{ return !(*this == val); }

		auto const& value() const { return m_value; }

	private:
		explicit field_name() = default;
		explicit field_name(std::string&& string):m_value{std::move(string)}{}
		std::string m_value;
	};

	class field_value
	{
	public:
		field_value() = default;

		static std::optional<field_value> create(std::string&& str)
		{
			if(str.empty())
			{ return field_value{}; }

			if(is_not_printable(str.front()) || is_not_printable(str.back()))
			{ return std::nullopt; }

			if(std::ranges::any_of(str, [](auto val){
				return is_not_printable(val) && !is_strict_whitespace(val);
			}))
			{ return std::nullopt; }

			return field_value{std::move(str)};
		}

		auto operator<=>(field_value const&) const = default;

		auto operator<=>(std::string_view val) const
		{ return m_value <=> val; }

		bool operator==(std::string_view val) const
		{ return m_value == val; }

		bool operator!=(std::string_view val) const
		{ return m_value != val; }

		auto const& value() const { return m_value; }

	private:
		explicit field_value(std::string&& string):m_value{std::move(string)}{}
		std::string m_value;
	};

	class field_map
	{
	public:
		field_map& append(field_name&& key, field_value const& value)
		{
			// TODO: In first case, we should be able to move value, but we still need to append
			//       `value` if `key` already exists
			auto ip = m_fields.emplace(std::move(key), value.value());
			if(ip.second)
			{ return *this; }

			auto& val = ip.first->second;
			if(value.value().size() != 0)
			{ val.append(", ").append(value.value()); }
			return *this;
		}

		field_map& append(std::string&& key, std::string&& value)
		{
			auto validated_key = field_name::create(std::move(key));
			auto validated_value = field_value::create(std::move(value));
			if(!validated_key || !validated_value)
			{ throw std::runtime_error{"Tried to write an invalid field name or field value to a HTTP header"}; }

			return append(std::move(*validated_key), *validated_value);
		}

		auto begin() const
		{ return std::begin(m_fields); }

		auto end() const
		{ return std::end(m_fields); }

		auto find(std::string_view field_name) const
		{ return m_fields.find(field_name); }

		[[nodiscard]] bool empty() const
		{ return m_fields.empty(); }

	private:
		std::map<field_name, std::string, std::less<>> m_fields;
	};

	struct request_line
	{
		request_method method;
		uri request_target;
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