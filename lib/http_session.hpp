#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_read_request_header.hpp"

#include <variant>

namespace west::http
{
	class write_error_response
	{
	public:
		template<io::data_sink Sink, request_handler RequestHandler, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>&,
 			Sink&,
			RequestHandler&)
		{
			return session_state_response{
				.status = session_state_status::io_error,
				.http_status = status::not_implemented,
				.error_message = make_unique_cstr("Not implemented")
			};
		}
	};

	enum class session_status{completed, more_data_needed, io_error};

	template<io::socket Socket, request_handler RequestHandler>
	class session
	{
	public:
		explicit session(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_connection{std::move(connection)},
			m_request_handler{std::move(req_handler)},
			m_req_content_length{0},
			m_conn_keep_alive{true}
		{}

		auto socket_is_ready()
		{
			std::array<char, 65536> buffer;
			io::buffer_view buff_view{buffer};
			while(true)
			{
				auto const res = std::visit([&buff_view, this](auto& state){
					return state(buff_view, m_connection, m_request_handler);
				}, m_state);

				switch(res.status)
				{
					case session_state_status::completed:
					{
						if(auto state = std::get_if<read_request_header>(&m_state); state != nullptr)
						{
							m_req_content_length = state->get_content_length();
							m_conn_keep_alive = state->get_keep_alive();
						}

						break;
					}

					case session_state_status::more_data_needed:
						return session_status::more_data_needed;

					case session_state_status::client_error_detected:
						// TODO: go to state for writing error report
						m_connection.stop_reading();
						m_state = write_error_response{};
						break;

					case session_state_status::io_error:
						return session_status::io_error;
				}
			}
		}

		bool get_conn_keep_alive() const
		{ return m_conn_keep_alive; }

		size_t get_req_content_length() const
		{ return m_req_content_length; }

	private:
		Socket m_connection;
		RequestHandler m_request_handler;
		std::variant<read_request_header, write_error_response> m_state;
		size_t m_req_content_length;
		bool m_conn_keep_alive;
	};
}

#endif
