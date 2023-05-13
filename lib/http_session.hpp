#ifndef WEST_HTTP_SESSION_HPP
#define WEST_HTTP_SESSION_HPP

#include "./http_read_request_header.hpp"

#include <variant>

namespace west::http
{
	template<io::socket Socket, request_handler RequestHandler, size_t BufferSize = 65536>
	class session
	{
	public:
		explicit session(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_connection{std::move(connection)},
			m_request_handler{std::move(req_handler)}
		{}

		void socket_is_ready()
		{
			std::array<char, BufferSize> buffer;
			io::buffer_view buff_view{buffer};
			while(true)
			{
				auto const res = std::visit([&buff_view, this](auto& state){
					return state(buff_view, m_connection, m_request_handler);
				}, m_state);

				switch(res.status)
				{
					case session_state_status::completed:
						// TODO: go to next state
						break;

					case session_state_status::more_data_needed:
						// TODO: Notify caller that operation should continue when data
						//       is available
						return;

					case session_state_status::client_error_detected:
						// TODO: go to state for writing error report
						m_connection.stop_reading();
						return;

					case session_state_status::io_error:
						// TODO: Notify that operation should not continue
						return;
				}
			}
		}

	private:
		Socket m_connection;
		RequestHandler m_request_handler;
		std::variant<read_request_header> m_state;
	};
}

#endif
