#include "ws_client.h"

#include "common_logger.h"

// timer function
void ws_timer_func(void *arg)
{
    WSClient *client = (WSClient *)arg;
    client->on_timer();
}

//the websocket event handler function
void ws_event_handler(struct mg_connection *conn, int event, void *event_data, void *func_data)
{
    WSClient *client = (WSClient *)func_data;
    client->on_event_handle(conn, event, event_data);
}

/////////////////////////////////////////////////////////////////////////////////////

WSClient::WSClient(const std::string &serverURL)
    : m_connection(NULL), m_server_url(serverURL),
      m_recv_func(NULL), m_recv_func_arg(NULL),
      m_ws_func(NULL), m_ws_func_arg(NULL), m_connected(false)
{
#ifdef _WIN32
#else
    pthread_mutex_init(&m_mutex, NULL);
#endif

    mg_mgr_init(&m_mgr);

    unsigned int timerOpts = MG_TIMER_REPEAT | MG_TIMER_RUN_NOW;
    mg_timer_init(&m_timer, 1000 * 12, timerOpts, ws_timer_func, this);
}

WSClient::~WSClient()
{
#ifdef _WIN32
#else
    pthread_mutex_destroy(&m_mutex);
#endif

    mg_mgr_free(&m_mgr);
    mg_timer_free(&m_timer);
}

void WSClient::on_timer()
{
    if (m_connected)
    {
#ifdef _WIN32
        std::unique_lock<std::mutex> lock(m_mutex);
        mg_ws_send(m_connection, "ping", 4, WEBSOCKET_OP_PING);
#else
        pthread_mutex_lock(&m_mutex);
        mg_ws_send(m_connection, "ping", 4, WEBSOCKET_OP_PING);
        pthread_mutex_unlock(&m_mutex);
#endif
	}
	else if (m_connection)
	{
		mg_close_conn(m_connection);
		m_connection = NULL;

		//NOTE: this function does not connect to peer, it allocates required resources and
		//starts the connect process. Once peer is really connected, the MG_EV_CONNECT event
		//is sent to connection event handler
		m_connection = mg_ws_connect(&m_mgr, m_server_url.c_str(), ws_event_handler, this, "%s", "Sec-WebSocket-Protocol: ctrl_protocol\r\n");
	}
	else
	{
		m_connection = mg_ws_connect(&m_mgr, m_server_url.c_str(), ws_event_handler, this, "%s", "Sec-WebSocket-Protocol: ctrl_protocol\r\n");
	}
}

void WSClient::on_event_handle(struct mg_connection *conn, int event, void *event_data)
{
    if (event == MG_EV_WS_OPEN)
    {
        // When websocket handshake is successful
        LOG_INFO("The websocket connection established");
        this->m_connected = true;

        if (m_ws_func)
        {
            m_ws_func(MG_EV_WS_OPEN, m_ws_func_arg);
        }
    }
    else if (event == MG_EV_WS_MSG)
    {
        // When we get response
        struct mg_ws_message *wm = (struct mg_ws_message *)event_data;
        if ((wm->flags & 0x0F) == WEBSOCKET_OP_TEXT)
        {
            //receive text
            on_message_received(wm->data.ptr, wm->data.len);
        }
    }
    else if (event == MG_EV_CLOSE)
    {
        // When websocket close
        LOG_INFO("The websocket connection closed");
        this->m_connection = NULL;
        this->m_connected = false;

        if (m_ws_func)
        {
            m_ws_func(MG_EV_CLOSE, m_ws_func_arg);
        }
    }
    else if (event == MG_EV_ERROR)
    {
        // When websocket error occurs
        LOG_INFO("The websocket connection error occurs:%s", (char *)event_data);
    }
    else if (event == MG_EV_WS_CTL)
    {
        //the websocket control message
        struct mg_ws_message *wm = (struct mg_ws_message *)event_data;
        uint8_t op = wm->flags & 0x0F;
        if (op == WEBSOCKET_OP_PING)
        {
            // LOG_INFO("The websocket receive: ping");
        }
        else if (op == WEBSOCKET_OP_PONG)
        {
            // LOG_INFO("The websocket receive: pong");
        }
    }
}

void WSClient::on_message_received(const char *data, size_t len)
{
    if (m_recv_func)
    {
        m_recv_func(data, len, this->m_recv_func_arg);
    }
}

void WSClient::set_text_received_func(WSReceiveTextCallback func, void *arg)
{
    this->m_recv_func = func;
    this->m_recv_func_arg = arg;
}

void WSClient::set_ws_event_func(WSEventCallback func, void* arg)
{
    this->m_ws_func = func;
    this->m_ws_func_arg = arg;
}

void WSClient::pulse(int timeout_ms)
{
    if (timeout_ms > 1000)
    {
        timeout_ms = 1000;
    }

    mg_mgr_poll(&m_mgr, timeout_ms);
}

bool WSClient::send_text(const std::string &msg)
{
    if (m_connection && m_connected)
    {
#ifdef _WIN32
        std::unique_lock<std::mutex> lock(m_mutex);
        size_t len = mg_ws_send(m_connection, msg.c_str(), msg.size(), WEBSOCKET_OP_TEXT);
        //the returned len includes the websocket header length, so it is greater than msg.size(),
#else
        pthread_mutex_lock(&m_mutex);
        size_t len = mg_ws_send(m_connection, msg.c_str(), msg.size(), WEBSOCKET_OP_TEXT);
        //the returned len includes the websocket header length, so it is greater than msg.size(),
        pthread_mutex_unlock(&m_mutex);
#endif

        return true;
    }

    return false;
}

bool WSClient::is_connected() const
{
    return m_connected;
}