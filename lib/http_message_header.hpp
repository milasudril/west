#ifndef WEST_HTTP_MESSAGE_HEADER_HPP
#define WEST_HTTP_MESSAGE_HEADER_HPP

#include <string>
#include <map>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <ranges>

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

	constexpr bool is_delimiter(char ch)
	{
		return ch == '"' || ch == '(' || ch == ')' || ch == ',' || ch == '/' || ch == ':'
			|| ch == ';' || ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '@'
			|| ch == '[' || ch == '\\' || ch == ']' || ch == '{' || ch == '}' || ch == 127;
	}

	constexpr bool is_whitespace(char ch)
	{ return (ch >= '\0' && ch <= ' ')  || ch == 127; }

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

		if(std::ranges::any_of(str, [](auto val){return (val & 0x80) || is_delimiter(val);}))
		{return false;}

		return true;
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

		static std::optional<field_value> create(std::string_view str)
		{
			if(std::ranges::any_of(str, [](auto val){
				return is_whitespace(val) && val != ' ' && val != '\t';
			}))
			{return std::nullopt;}

			auto first_non_ws = std::ranges::find_if_not(str, is_whitespace);
			if(first_non_ws == std::end(str))
			{ return field_value{}; }

			auto last_non_ws = std::ranges::find_if_not(std::ranges::reverse_view{str}, is_whitespace);

			return field_value{std::string{first_non_ws, last_non_ws.base()}};
		}

		auto operator<=>(field_value const&) const = default;

		auto operator<=>(std::string_view val) const
		{ return m_value <=> val; }

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