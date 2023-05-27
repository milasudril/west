#ifndef WEST_HTTP_REQUEST_PROCESSOR_HPP
#define WEST_HTTP_REQUEST_PROCESSOR_HPP

#include "./http_request_state_transitions.hpp"
#include "./io_adapter.hpp"

namespace west::http
{
	enum class request_processor_status{completed, more_data_needed, application_error, io_error};

	template<io::socket Socket, request_handler RequestHandler>
	class request_processor
	{
		using buffer_type = std::array<char, 65536>;

	public:
		explicit request_processor(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_session{std::move(connection), std::move(req_handler), request_header{}, response_header{}},
			m_recv_buffer{std::make_unique<buffer_type>()},
			m_send_buffer{std::make_unique<buffer_type>()},
			m_buff_spans{buffer_span{*m_recv_buffer}, buffer_span{*m_send_buffer}}
		{}

		auto socket_is_ready()
		{
			while(true)
			{
				auto const res = std::visit([this]<class T>(T& state){
					return state(std::get<select_buffer_index<T>::value>(m_buff_spans), m_session);
				}, m_state);

				switch(res.status)
				{
					case session_state_status::completed:
						m_state = make_state_handler(m_state, m_session.request_header, m_session.response_header);
						break;

					case session_state_status::connection_closed:
						return request_processor_status::completed;

					case session_state_status::more_data_needed:
						return request_processor_status::more_data_needed;

					case session_state_status::client_error_detected:
						m_session.connection.stop_reading();
						break;

					case session_state_status::write_response_failed:
						return request_processor_status::application_error;

					case session_state_status::io_error:
						return request_processor_status::io_error;
				}
			}
		}

		bool get_conn_keep_alive() const
		{ return m_session.conn_keep_alive; }

		size_t get_req_content_length() const
		{ return m_session.req_content_length; }

		auto const& get_request_handler() const
		{ return m_session.request_handler; }

	private:
		session<Socket, RequestHandler> m_session;
		request_state_holder m_state;
		std::unique_ptr<buffer_type> m_recv_buffer;
		std::unique_ptr<buffer_type> m_send_buffer;

		using buffer_span = io_adapter::buffer_span<buffer_type::value_type, std::tuple_size_v<buffer_type>>;
		std::array<buffer_span, 2> m_buff_spans;
	};
}
#endif
