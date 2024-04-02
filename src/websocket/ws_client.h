#ifndef _H_WEBSOCKET_CLIENT_H_
#define _H_WEBSOCKET_CLIENT_H_

#include <string>
#include <stdint.h>
#ifdef _WIN32
#include <mutex>
#else
#include <pthread.h>
#endif

#include "mongoose.h"

//the callback function for websocket text received
// @param data -- the data
// @param len -- the data length
// @param arg -- the user argument
typedef void (*WSReceiveTextCallback)(const char *data, size_t len, void* arg);

//the callback function for websocket event
//@param event -- the event
//@param arg -- the user argument
typedef void (*WSEventCallback)(int event, void* arg);

//the websocket client, it is NOT thread-safe
class WSClient
{
    friend void ws_timer_func(void *arg);
    friend void ws_event_handler(struct mg_connection *conn, int event, void *event_data, void *func_data);

public:
    WSClient(const std::string &serverURL);
    ~WSClient();

    /**
     * @brief Set the text received function
     * 
     * @param func -- the callback function
     * @param arg -- the argument of the callback function
     */
    void set_text_received_func(WSReceiveTextCallback func, void* arg);

    /**
     * @brief Set the websocket event callback function
     * @param func -- the callback function
     * @param arg -- the argument of the callback function
     */
    void set_ws_event_func(WSEventCallback func, void* arg);

    /**
     * @brief send text message
     * 
     * @param msg -- the message
     * @return bool
     */
    bool send_text(const std::string &msg);

    /**
     * @brief pulse 
     * 
     * @param timeout_ms the timeout in milliseconds, it must be less than 1000
     */
    void pulse(int timeout_ms);

    /**
     * @brief is the websocket client connected to server?
     * 
     * @return true 
     * @return false 
     */
    bool is_connected() const;

private:
    /**
     * @brief the timer function, user 
     * 
     */
    void on_timer();

    /**
     * @brief the websocket event handler function
     * 
     * @param conn -- the connection
     * @param event -- the websocket event
     * @param event_data -- the websocket event data pointer
     */
    void on_event_handle(struct mg_connection *conn, int event, void *event_data);

    /**
     * @brief the websocket received text message
     * 
     * @param data -- the message
     * @param len -- the message length
     */
    void on_message_received(const char *data, size_t len);

private:
    //the websocket manager
    struct mg_mgr m_mgr;
    //the timer
    struct mg_timer m_timer;
    //the websocket connection
    struct mg_connection *m_connection;

    //the websocket server url address
    std::string m_server_url;

    //the recv callback function
    WSReceiveTextCallback m_recv_func;
    //the argument of the recv callback function
    void* m_recv_func_arg;
    //the websocket event function
    WSEventCallback m_ws_func;
    //the argument of the websocket event function
    void* m_ws_func_arg;
    //if the websocket is connected
    bool m_connected;

    //the mutex for thread safe
#ifdef _WIN32
	std::mutex m_mutex;
#else
    pthread_mutex_t m_mutex;
#endif
};

#endif