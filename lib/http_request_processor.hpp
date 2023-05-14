#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_read_request_header.hpp"

#include <variant>

namespace west::http
{
	class write_error_response
	{
	public:
		template<class T, size_t BufferSize>
		[[nodiscard]] auto operator()(io::buffer_view<char, BufferSize>&, T const&)
		{
			return session_state_response{
				.status = session_state_status::io_error,
				.http_status = status::not_implemented,
				.error_message = make_unique_cstr("Not implemented")
			};
		}
	};

	enum class request_processor_status{completed, more_data_needed, io_error};

	template<io::socket Socket, request_handler RequestHandler>
	class request_processor
	{
	public:
		explicit request_processor(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_session{std::move(connection), std::move(req_handler)}
		{}

		auto socket_is_ready()
		{
			std::array<char, 65536> buffer;
			io::buffer_view buff_view{buffer};
			while(true)
			{
				auto const res = std::visit([&buff_view, this](auto& state){
					return state(buff_view, m_session);
				}, m_state);

				switch(res.status)
				{
					case session_state_status::completed:
					{
						//m_state = initiate_next_state(m_state, m_session);
						break;
					}

					case session_state_status::more_data_needed:
						return request_processor_status::more_data_needed;

					case session_state_status::client_error_detected:
						// TODO: go to state for writing error report
						m_session.connection.stop_reading();
						m_state = write_error_response{};
						break;

					case session_state_status::io_error:
						return request_processor_status::io_error;
				}
			}
		}

		bool get_conn_keep_alive() const
		{ return m_session.conn_keep_alive; }

		size_t get_req_content_length() const
		{ return m_session.req_content_length; }

	private:
		session<Socket, RequestHandler> m_session;
		std::variant<read_request_header, write_error_response> m_state;
	};
}

#endif
