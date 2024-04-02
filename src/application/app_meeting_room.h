#ifndef _H_APP_MEETING_ROOM_H_
#define _H_APP_MEETING_ROOM_H_

#include <map>
#include <set>
#include <string>

#ifdef _WIN32
#include <mutex>
#include <thread>
#include <chrono>
#else
#include <pthread.h>
#endif

#include <stdint.h>
#include <time.h>

#include "common_json.h"
#include "rtp_session_audio.h"
#include "rtp_session_video.h"
#include "ws_client.h"
#include "app_util.h"
#include "app_room_user.h"

//the conference event callback function
//@param event -- the event 
//@param data -- the callback data
//@param userArg -- the user argument
typedef void(*ConferenceEventCallback)(int event, void* data, void* userArg);

//the conference websocket callback function
//@param event -- the event
//@param userArg -- the user argument
typedef void(*ConferenceSignalCallback)(int event, void* userArg);

//the video live meeting room
class LiveMeetingRoom
{
private:
	LiveMeetingRoom();

public:
	virtual ~LiveMeetingRoom();

	/**
	 * @brief Get the singleton instance
	 * 
	 * @return LiveMeetingRoom* 
	 */
	static LiveMeetingRoom *get_instance();

	/**
	 * @brief initialize
	 * 
	 * @param websocket_ip -- the server websocket ip address
	 * @param websocket_port -- the server websocket port
	 * @return true -- successful
	 * @return false -- failed
	 */
	bool initialize(const std::string &websocket_ip, uint16_t websocket_port);

	/**
	 * @brief un initialize
	 * 
	 */
	void un_initialize();
	
	/**
	 * @brief check if the websocket is connected
	 * 
	 * @return true 
	 * @return false 
	 */
	bool is_signal_connected();

	/**
	 * @brief get the room id
	 * @return
	 */
	std::string get_room_id() const
	{
		return m_my_room_id;
	}

	/**
	* @brief get the ip address which the server detects
	* @return
	*/
	std::string get_my_ip_addr() const
	{
		return m_my_ip_addr;
	}

	/**
	* @brief get my user id
	* @return
	*/
	std::string get_my_user_id() const
	{
		return m_my_user_id;
	}

	/**
	* @brief get my user name
	* @return
	*/
	std::string get_my_user_name() const
	{
		return m_my_user_name;
	}

	/**
	* @brief get my user uuid
	* @return
	*/
	std::string get_my_user_uuid() const
	{
		return m_my_user_uuid;
	}

	/**
	* @brief get the video rtp ssrc
	* @return
	*/
	uint32_t get_video_ssrc() const
	{
		return m_video_ssrc;
	}

	/**
	* @brief get the audio rtp ssrc
	* @return
	*/
	uint32_t get_audio_ssrc() const
	{
		return m_audio_ssrc;
	}

	/**
	* @brief get the push video ip address
	* @return
	*/
	std::string get_video_push_ip() const
	{
		return m_video_push_ip;
	}

	/**
	* @brief get the push video port
	* @return
	*/
	uint16_t get_video_push_port() const
	{
		return m_video_push_port;
	}

	/**
	* @brief get the push audio ip
	* @return
	*/
	std::string get_audio_push_ip() const
	{
		return m_audio_push_ip;
	}

	/**
	* @brief get the push audio port
	* @return
	*/
	uint16_t get_audio_push_port() const
	{
		return m_audio_push_port;
	}

	/**
	* @brief send h.264 data to the server
	* @param data -- the data
	* @param length -- the data length
	*/
	void send_h264_data(const uint8_t *data, size_t length);

	/**
	* @brief send aac data to the server
	* @param data -- the data
	* @param length -- the data length
	*/
	void send_aac_data(const uint8_t *data, size_t length);

	/**
	 * @brief Set the h264 receive callback function
	 * @param func -- the H.264 data receive function
	 * @param arg -- the user argument
	 */
	void set_h264_receive_callback(OnH264ReceiveCallback func, void* arg);

	/**
	 * @brief Set the aac receive callback function
	 * @param func -- the AAC data receive function
	 * @param arg -- the user argument
	 */
	void set_aac_receive_callback(OnAACReceiveCallback func, void* arg);

	/**
	 * @brief send create-conference signal to the server
	 * 
	 * @param myUserId -- my user id
	 * @param myUserName -- my user name
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_create_conference(const std::string &myUserId, const std::string &myUserName);

	/**
	 * @brief send join-conference signal to the server
	 * 
	 * @param conferenceId -- the conference id
	 * @param myUserId -- my user id
	 * @param myUserName -- my user name
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_join_conference(const std::string &conferenceId,
								const std::string &myUserId, const std::string &myUserName);

	/**
	 * @brief send start-pull-stream signal to the server
	 * @param streams -- the streams, key: the user uuid, value: the ssrs set
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_start_pull_stream(std::map<std::string, std::set<std::string>> &streams);

	/**
	 * @brief send stop-pull-stream signal to the server
	 * @param streams -- the streams, key: the user uuid, value: the ssrs set
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_stop_pull_stream(std::map<std::string, std::set<std::string>> &streams);

	/**
	 * @brief send exit-conference signal to the server
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_exit_conference();

	/**
	 * @brief send stop-conference signal to the server
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_stop_conference();

	/**
	 * @brief send online-users signal to the server
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_online_users();

	/**
	 * @brief send heartbeat signal to the server
	 * 
	 * @param myUserUUID -- my users uuid
	 * @param conferenceID -- the conference id
	 * @return int, 0 on success, otherwise a negative value on error
	 */
	int signal_heartbeat(const std::string &myUserUUID,
						  const std::string &conferenceID);
						

	/**
	 * @brief Set the conference event callback function
	 * 
	 * @param func -- the callback function
	 * @param arg -- the argument
	 */
	void set_event_callback_func(ConferenceEventCallback func, void* arg);

	/**
	 * @brief Set the websocket signal callback function
	 * 
	 * @param func -- the callback function
	 * @param arg -- the argument
	 */
	void set_signal_callback_func(ConferenceSignalCallback func, void* arg);

	/**
	 * @brief websocket message received callback function. the function should not
	 * invoked by users
	 * 
	 * @param message -- the websocket message
	 */
	void on_websocket_message(const std::string &message);

	/**
	 * @brief websocket event callback function. the function should not
	 * invoked by users
	 * 
	 * @param event -- the websocket event
	 */
	void on_websocket_event(int event);

	/**
	 * @brief the websocket pulse, the user shound not invoke this function
	 * 
	 */
	void websocket_pulse_loop();

	/**
	 * @brief the receive loop, the user shound not invoke this function
	 * 
	 */
	void receive_loop();

private:
	void free_context();

	/**
	*@brief stop receiving rtp
	*/
	void stop_receive();

	/**
	* @brief add the audio/video server destination
	* @param majorVideoIP -- the major video ip address of destination
	* @param majorVideoPort -- the major video port of destination
	* @param majorVideoSequenceStart -- the start sequence of major video session
	* @param majorVideoTimestampStart -- the start timestamp of major video session
	* @param majorVideoSSRC -- the ssrc of major video session
	* @param audioIP -- the audio ip address of destination
	* @param audioPort -- the audio port of destination
	* @param audioSequenceStart -- the start sequence of audio session
	* @param audioTimestampStart -- the start timestamp of audio session
	* @param audioSSRC -- the ssrc of audio session
	*/
	void add_sender_destination(const std::string &majorVideoIP,
								uint16_t majorVideoPort,
								uint16_t majorVideoSequenceStart,
								uint32_t majorVideoTimestampStart,
								uint32_t majorVideoSSRC,
								const std::string &audioIP,
								uint16_t audioPort,
								uint16_t audioSequenceStart,
								uint32_t audioTimestampStart,
								uint32_t audioSSRC);

	/**
	 * @brief remove sender
	 * 
	 */
	void remove_sender();

	/**
	 * @brief create the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_create(JsonObject &params);

	/**
	 * @brief join the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_join(JsonObject &params);

	/**
	 * @brief new user joined the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_new_joined(JsonObject &params);

	/**
	 * @brief start to pull streams from server
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_pull_stream(JsonObject &params);

	/**
	 * @brief stop pulling streams from server
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_stop_pulling(JsonObject &params);

	/**
	 * @brief exit the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_exit(JsonObject &params);

	/**
	 * @brief a new has gone out of the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_user_gone(JsonObject &params);

	/**
	 * @brief stop the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_stop(JsonObject &params);

	/**
	 * @brief query the online users in the conference
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_online_users(JsonObject &params);

	/**
	 * @brief heartbeat
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_heartbeat(JsonObject &params);

	/**
	 * @brief the conference is been closing
	 * @param params -- the parameters of websocket message
	 */
	void on_ws_conference_closing(JsonObject &params);

private:
	//initialized or not
	bool m_initialized;
#ifdef _WIN32
	//the websocket thread
	std::thread m_websocket_thread;
#else
	//the websocket thread
	pthread_t m_websocket_thread;
#endif

	//whether the websocket thread running
	bool m_ws_thread_running;
	//the websocket
	WSClient *m_websocket_client;
	
	//the users map
	//key: the user uuid, value: other RoomUsers in the meeting room
	std::map<std::string, RoomUser *> m_other_users_map;
	//whether the receiving thread running
	bool m_receive_thread_running;
#ifdef _WIN32
	//the user receive thread
	std::thread m_receive_thread;
	//the mutex for receive
	std::mutex m_receive_mutex;
#else
	//the user receive thread
	pthread_t m_receive_thread;
	//the mutex for receive
	pthread_mutex_t m_receive_mutex;
#endif

	//the audio rtp send session, send audio rtp data to server
	RTPSessionAudio *m_rtp_audio_sender;
	//the video rtp send session, send video rtp data to server
	RTPSessionVideo *m_rtp_video_sender;
	//whether the rtp send session initialized
	bool m_rtp_send_initialized;

	//the conference room id which the server generates
	std::string m_my_room_id;
	//the ip address which the server detects
	std::string m_my_ip_addr;
	//my user id
	std::string m_my_user_id;
	//my user name
	std::string m_my_user_name;
	//my user uuid
	std::string m_my_user_uuid;
	//the video rtp ssrc
	uint32_t m_video_ssrc;
	//the audio rtp ssrc
	uint32_t m_audio_ssrc;
	//the push video ip address
	std::string m_video_push_ip;
	//the push video port
	uint16_t m_video_push_port;
	//the push audio ip address
	std::string m_audio_push_ip;
	//the push audio port
	uint16_t m_audio_push_port;

	//the event callback function
	ConferenceEventCallback m_event_callback_func;
	//the event callback argument
	void* m_event_callback_arg;

	//the websocket signal callback function
	ConferenceSignalCallback m_signal_callback_func;
	//the websocket signal argument
	void* m_signal_callback_arg;

	//is the room processing signal ?
	bool m_is_signal_running;
	//the previous signal time in seconds
	uint64_t m_prev_signal_time;

	//the H.264 received callback
	OnH264ReceiveCallback m_h264_callback;
	//the H.264 received callback argument
	void* m_h264_callback_arg;

	//the AAC received callback
	OnAACReceiveCallback m_aac_callback;
	//the AAC received callback argument
	void* m_aac_callback_arg;
};

#endif
