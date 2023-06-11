#ifndef WEST_HTTP_REQUEST_PROCESSOR_HPP
#define WEST_HTTP_REQUEST_PROCESSOR_HPP

#include "./http_request_state_transitions.hpp"
#include "./io_adapter.hpp"

namespace west::http
{
	enum class request_processor_status{completed, more_data_needed, application_error, io_error};

	struct process_request_result
	{
		request_processor_status status;
		session_state_io_direction io_dir;

		constexpr bool operator==(process_request_result const&) const = default;
		constexpr bool operator!=(process_request_result const&) const = default;
	};

	template<io::socket Socket, request_handler RequestHandler>
	class request_processor
	{
		using buffer_type = std::array<char, 65536>;

	public:
		explicit request_processor(Socket&& connection, RequestHandler&& req_handler = RequestHandler{}):
			m_session{std::move(connection), std::move(req_handler), request_info{}, response_header{}},
			m_recv_buffer{std::make_unique<buffer_type>()},
			m_send_buffer{std::make_unique<buffer_type>()},
			m_buff_spans{buffer_span{*m_recv_buffer}, buffer_span{*m_send_buffer}}
		{}

		[[nodiscard]] auto socket_is_ready()
		{
			while(true)
			{
				auto res = std::visit([this]<class T>(T& state){
					return state.socket_is_ready(std::get<select_buffer_index<T>::value>(m_buff_spans), m_session);
				}, m_state.first);

				switch(res.status)
				{
					case session_state_status::completed:
						m_state = make_state_handler(m_state.first, m_session.request_info, m_session.response_info);
						break;

					case session_state_status::connection_closed:
						return process_request_result{
							request_processor_status::completed,
							m_state.second
						};

					case session_state_status::more_data_needed:
						return process_request_result{
							request_processor_status::more_data_needed,
							m_state.second
						};

					case session_state_status::client_error_detected:
					{
						m_session.connection.stop_reading();

						m_session.response_info = response_info{};
						auto const saved_http_status = res.state_result.http_status;
						m_session.request_handler.finalize_state(m_session.response_info.header.fields,
							std::move(res.state_result));
						m_session.response_info.header.status_line.http_version = version{1, 1};
						m_session.response_info.header.status_line.status_code = saved_http_status;
						m_session.response_info.header.status_line.reason_phrase = to_string(saved_http_status);

						m_state = std::pair{
							write_response_header{m_session.response_info.header},
							session_state_io_direction::output
						};
						break;
					}

					case session_state_status::write_response_failed:
						return process_request_result{
							request_processor_status::application_error,
							m_state.second
						};

					case session_state_status::io_error:
						return process_request_result{
							request_processor_status::io_error,
							m_state.second
						};
				}
			}
		}

		[[nodiscard]] auto socket_is_idle()
		{
			return process_request_result{
				request_processor_status::completed,
				m_state.second
			};
		}

		auto& session()
		{ return m_session; }

		auto const& session() const
		{ return m_session; }

	private:
		struct session<Socket, RequestHandler> m_session;
		std::pair<request_state_holder, session_state_io_direction> m_state;
		std::unique_ptr<buffer_type> m_recv_buffer;
		std::unique_ptr<buffer_type> m_send_buffer;

		using buffer_span = io_adapter::buffer_span<buffer_type::value_type, std::tuple_size_v<buffer_type>>;
		std::array<buffer_span, 2> m_buff_spans;
	};

	constexpr bool is_session_terminated(request_processor_status status)
	{
		return status == request_processor_status::io_error
			|| status == request_processor_status::application_error
			|| status == request_processor_status::completed;
	}

	constexpr bool is_session_terminated(process_request_result res)
	{ return is_session_terminated(res.status); }
}
#endif
