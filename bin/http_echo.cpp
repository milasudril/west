//@	{"target":{"name": "http_echo.o"}}

#include "lib/io_inet_server_socket.hpp"
#include "lib/service_registry.hpp"
#include "lib/http_request_handler.hpp"
#include "lib/http_request_processor.hpp"

#include <string>

namespace
{
	enum class request_handler_error_code{no_error, would_block, error};

	struct read_result
	{
		size_t bytes_read;
		request_handler_error_code ec;
	};

	constexpr bool can_continue(request_handler_error_code ec)
	{
		switch(ec)
		{
			case request_handler_error_code::no_error:
				return true;
			case request_handler_error_code::would_block:
				return true;
			case request_handler_error_code::error:
				return false;
			default:
				__builtin_unreachable();
		}
	}

	constexpr bool is_error_indicator(request_handler_error_code ec)
	{
		switch(ec)
		{
			case request_handler_error_code::no_error:
				return false;
			case request_handler_error_code::would_block:
				return false;
			case request_handler_error_code::error:
				return true;
			default:
				__builtin_unreachable();
		}
	}

	constexpr char const* to_string(request_handler_error_code ec)
	{
		switch(ec)
		{
			case request_handler_error_code::no_error:
				return "No error";
			case request_handler_error_code::would_block:
				return "Would block";
			case request_handler_error_code::error:
				return "Error";
			default:
				__builtin_unreachable();
		}
	}

	struct request_handler_write_result
	{
		size_t bytes_written;
		request_handler_error_code ec;
	};

	struct request_handler_read_result
	{
		size_t bytes_read;
		request_handler_error_code ec;
	};
	
	
	class http_request_handler
	{
	public:
		auto finalize_state(west::http::request_header const& header)
		{
			fprintf(stderr, "Finalize read request header\n");
			fflush(stderr);
			
			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = nullptr;

			m_response_body = header.request_line.method.value();
			m_response_body
				.append(" ")
				.append(header.request_line.request_target.value())
				.append(" ")
				.append("HTTP/")
				.append(to_string(header.request_line.http_version))
				.append("\r\n");

			for(auto& item : header.fields)
			{ m_response_body.append(item.first.value()).append(": ").append(item.second).append("\r\n"); }

			m_response_body.append("\r\n");

			return validation_result;
		}

		auto finalize_state(west::http::field_map& fields)
		{
			fprintf(stderr, "Finalize read request body\n");
			fflush(stderr);
			
			fields.append("Content-Length", std::to_string(std::size(m_response_body)))
				.append("Content-Type", "text/plain");
				
			m_read_offset = std::begin(m_response_body);

			west::http::finalize_state_result validation_result;
			validation_result.http_status = west::http::status::ok;
			validation_result.error_message = nullptr;
			return validation_result;
		}

		void finalize_state(west::http::field_map& fields, west::http::finalize_state_result&& res)
		{
			fprintf(stderr, "Finalize for error state\n");
			fflush(stderr);
			
			m_error_message = std::move(res.error_message);
			m_response_body = std::string{m_error_message.get()};
			m_read_offset = std::begin(m_response_body);
			fields.append("Content-Length", std::to_string(std::size(m_response_body)));
		}

		auto process_request_content(std::span<char const> buffer)
		{
			fprintf(stderr, "Reading request content\n");
			fflush(stderr);
			
			m_response_body.insert(std::end(m_response_body), std::begin(buffer), std::end(buffer));
			return request_handler_write_result{
				.bytes_written = std::size(buffer),
				.ec = request_handler_error_code::no_error
			};
		}

		auto read_response_content(std::span<char> buffer)
		{
			fprintf(stderr, "Writing response body\n");
			fflush(stderr);
			
			auto const n_bytes_left = static_cast<size_t>(std::end(m_response_body) - m_read_offset);
			auto const bytes_to_read = std::min(std::size(buffer), n_bytes_left);
			std::copy_n(m_read_offset, bytes_to_read, std::begin(buffer));
			m_read_offset += bytes_to_read;

			return request_handler_read_result{
				bytes_to_read,
				request_handler_error_code::no_error
			};
		}

	private:
		std::unique_ptr<char[]> m_error_message;
		std::string m_response_body;
		std::string::iterator m_read_offset;
	};
	
	struct http_session_factory
	{
		auto create_session(west::io::inet_connection&& connection)
		{ return west::http::request_processor{std::move(connection), http_request_handler{}}; }
	};
	
	enum class adm_session_status{keep_connection, close_connection};
	
	constexpr bool is_session_terminated(adm_session_status status)
	{ return status == adm_session_status::close_connection; }
	
	template<class CallbackRegistry>
	struct adm_request_handler
	{		
		west::io::inet_connection connection;
		CallbackRegistry registry;
		
		auto socket_is_ready()
		{
			std::array<char, 65536> read_buffer{};
			auto res = connection.read(read_buffer);
			if(res.bytes_read == 0)
			{
				switch(res.ec)
				{
					case west::io::operation_result::operation_would_block:
						return adm_session_status::keep_connection;
					case west::io::operation_result::completed:
						return adm_session_status::close_connection;
					case west::io::operation_result::error:
						return adm_session_status::close_connection;
				}
			}
			else
			{
				if(std::string_view{std::data(read_buffer), res.bytes_read} == "shutdown")
				{ registry.clear(); }
			}
			
			return adm_session_status::keep_connection;
		}
	};

	template<class CallbackRegistry>
	struct adm_session_factory
	{
		auto create_session(west::io::inet_connection&& connection)
		{ return adm_request_handler{std::move(connection), registry}; }

		CallbackRegistry registry;
	};
}

template<>
struct west::session_state_mapper<west::http::request_processor_status>
{
	constexpr auto operator()(http::request_processor_status) const
	{ return io::listen_on::read_is_possible; }
};

template<>
struct west::session_state_mapper<adm_session_status>
{
	constexpr auto operator()(adm_session_status) const
	{ return io::listen_on::read_is_possible; }
};

int main()
{
	west::io::inet_address address{"127.0.0.1"};
	west::io::inet_server_socket http{
		address,
		std::ranges::iota_view{49152, 65536},
		128
	};
	west::io::inet_server_socket adm{
		address,
		std::ranges::iota_view{49152, 65536},
		128
	};

	printf("http %u\n"
		"adm %u\n",
		http.port(),
		adm.port()
	);
	fflush(stdout);
	
	west::service_registry services{};
	services
		.enroll(std::move(http), http_session_factory{})
		.enroll(std::move(adm), adm_session_factory{services.fd_callback_registry()})
		.process_events();
}
