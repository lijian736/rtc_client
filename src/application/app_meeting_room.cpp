#include "app_meeting_room.h"

#ifdef _WIN32
#include <functional>
#endif

#include "common_logger.h"
#include "common_utils.h"
#include "common_utf8.h"

#include "ws_client.h"
#include "app_room_user.h"
#include "app_command.h"
#include "app_event.h"
#include "app_error.h"

static void websocket_received(const char *data, size_t len, void *arg)
{
	LiveMeetingRoom *room = (LiveMeetingRoom *)arg;

	std::string message(data, len);
	room->on_websocket_message(message);
}

static void websocket_event(int event, void*arg)
{
	LiveMeetingRoom *room = (LiveMeetingRoom *)arg;

	room->on_websocket_event(event);
}

static void *websocket_thread_func(void *ptr)
{
	LiveMeetingRoom *room = (LiveMeetingRoom *)ptr;
	room->websocket_pulse_loop();

	return 0;
}

static void *receive_thread_func(void *ptr)
{
	LiveMeetingRoom *room = (LiveMeetingRoom *)ptr;
	room->receive_loop();

	return 0;
}

////////////////////////////////////////////////////////////

LiveMeetingRoom::LiveMeetingRoom()
{
	m_initialized = false;
	m_ws_thread_running = false;
	m_websocket_client = NULL;
	m_receive_thread_running = false;
	m_rtp_audio_sender = NULL;
	m_rtp_video_sender = NULL;
	m_rtp_send_initialized = false;

	m_event_callback_func = NULL;
	m_event_callback_arg = NULL;

	m_signal_callback_func = NULL;
	m_signal_callback_arg = NULL;

	m_is_signal_running = false;
	m_prev_signal_time = 0;
	m_aac_callback = NULL;
	m_aac_callback_arg = NULL;
	m_h264_callback = NULL;
	m_h264_callback_arg = NULL;

#ifdef _WIN32
#else
	pthread_mutex_init(&m_receive_mutex, NULL);
#endif
}

LiveMeetingRoom::~LiveMeetingRoom()
{
#ifdef _WIN32
#else
	pthread_mutex_destroy(&m_receive_mutex);
#endif
}

void LiveMeetingRoom::free_context()
{
	if (m_ws_thread_running)
	{
		m_ws_thread_running = false;
#ifdef _WIN32
		m_websocket_thread.join();
#else
		pthread_join(m_websocket_thread, NULL);
#endif
	}

	if (m_receive_thread_running)
	{
		m_receive_thread_running = false;
#ifdef _WIN32
		m_receive_thread.join();
#else
		pthread_join(m_receive_thread, NULL);
#endif
	}

	if (m_websocket_client)
	{
		LOG_DEBUG("delete m_websocket_client");
		delete m_websocket_client;
		m_websocket_client = NULL;
	}

	if (m_rtp_audio_sender)
	{
		LOG_DEBUG("delete m_rtp_audio_sender");
		delete m_rtp_audio_sender;
		m_rtp_audio_sender = NULL;
	}

	if (m_rtp_video_sender)
	{
		LOG_DEBUG("delete m_rtp_video_sender");
		delete m_rtp_video_sender;
		m_rtp_video_sender = NULL;
	}

	std::map<std::string, RoomUser *>::iterator it = m_other_users_map.begin();
	for (; it != m_other_users_map.end(); it++)
	{
		delete it->second;
	}
	m_other_users_map.clear();

	m_rtp_send_initialized = false;
	m_initialized = false;
}

void LiveMeetingRoom::un_initialize()
{
	free_context();
}

bool LiveMeetingRoom::is_signal_connected()
{
	if (m_websocket_client)
	{
		return m_websocket_client->is_connected();
	}
	else
	{
		return false;
	}
}

void LiveMeetingRoom::stop_receive()
{
	if (m_receive_thread_running)
	{
		m_receive_thread_running = false;
#ifdef _WIN32
		m_receive_thread.join();
#else
		pthread_join(m_receive_thread, NULL);
#endif
	}
}

LiveMeetingRoom *LiveMeetingRoom::get_instance()
{
	static LiveMeetingRoom instance;
	return &instance;
}

void LiveMeetingRoom::set_event_callback_func(ConferenceEventCallback func, void *arg)
{
	this->m_event_callback_func = func;
	this->m_event_callback_arg = arg;
}

void LiveMeetingRoom::set_signal_callback_func(ConferenceSignalCallback func, void *arg)
{
	this->m_signal_callback_func = func;
	this->m_signal_callback_arg = arg;
}

void LiveMeetingRoom::set_h264_receive_callback(OnH264ReceiveCallback func, void* arg)
{
	m_h264_callback = func;
	m_h264_callback_arg = arg;
}

void LiveMeetingRoom::set_aac_receive_callback(OnAACReceiveCallback func, void* arg)
{
	m_aac_callback = func;
	m_aac_callback_arg = arg;
}

void LiveMeetingRoom::websocket_pulse_loop()
{
	while (m_ws_thread_running)
	{
		m_websocket_client->pulse(50);
	}
}

void LiveMeetingRoom::receive_loop()
{
	uint32_t count = 0;
	while (m_receive_thread_running)
	{
		{
#ifdef _WIN32
			std::unique_lock<std::mutex> lock(m_receive_mutex);
#else
			pthread_mutex_lock(&m_receive_mutex);
#endif
			std::map<std::string, RoomUser *>::iterator it = m_other_users_map.begin();
			for (; it != m_other_users_map.end(); it++)
			{
				RoomUser *usr = it->second;
				usr->receive_audio();
				usr->receive_video();
				if (count % 2000 == 0)
				{
					usr->nat_pinhole();
				}
			}
#ifdef _WIN32
#else
			pthread_mutex_unlock(&m_receive_mutex);
#endif
			count++;
		}

		//sleep 4 milliseconds
#ifdef _WIN32
		std::this_thread::sleep_for(std::chrono::milliseconds(4));
#else
		usleep(1000 * 8);
#endif
	}
}

bool LiveMeetingRoom::initialize(const std::string &websocket_ip, uint16_t websocket_port)
{
	int ret;
	if (m_initialized)
	{
		free_context();
		m_initialized = false;
	}

	//create the websocket client
	std::string wsaddress = "ws://" + websocket_ip + ":" + common_to_string(websocket_port) + "/rtc_signal";

	LOG_DEBUG("websocket server:%s", wsaddress.c_str());
	m_websocket_client = new WSClient(wsaddress);
	if (!m_websocket_client)
	{
		goto exitFlag;
	}
	m_websocket_client->set_text_received_func(websocket_received, this);
	m_websocket_client->set_ws_event_func(websocket_event, this);

	//start the websocket thread
	m_ws_thread_running = true;
#ifdef _WIN32
	m_websocket_thread = std::thread(std::bind(&LiveMeetingRoom::websocket_pulse_loop, this));
#else
	ret = pthread_create(&m_websocket_thread, NULL, websocket_thread_func, this);
	if (ret != 0)
	{
		LOG_ERROR("Start websocket thread error.");
		m_ws_thread_running = false;
		goto exitFlag;
	}
#endif

	m_rtp_audio_sender = new (std::nothrow) RTPSessionAudio();
	if (!m_rtp_audio_sender)
	{
		goto exitFlag;
	}

	m_rtp_video_sender = new (std::nothrow) RTPSessionVideo();
	if (!m_rtp_video_sender)
	{
		goto exitFlag;
	}

	m_initialized = true;
	return true;

exitFlag:
	free_context();
	return false;
}

void LiveMeetingRoom::send_h264_data(const uint8_t *data, size_t length)
{
	if (!m_rtp_video_sender)
	{
		return;
	}

	m_rtp_video_sender->send_h264_data(data, length);
}

void LiveMeetingRoom::send_aac_data(const uint8_t *data, size_t length)
{
	if (!m_rtp_audio_sender)
	{
		return;
	}

	m_rtp_audio_sender->send_aac_data(data, length);
}

int LiveMeetingRoom::signal_create_conference(const std::string &myUserId, const std::string &myUserName)
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() != 0 || m_my_user_uuid.size() != 0)
	{
		return ERROR_CONFERENCE_ALREADY_JOINED;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}
	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	std::map<std::string, std::string> params;
	params["user_id"] = myUserId;
	params["user_name"] = myUserName;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_CREATE, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message TYPE_CONFERENCE_CREATE failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_join_conference(const std::string &conferenceId,
											const std::string &myUserId, const std::string &myUserName)
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() != 0 || m_my_user_uuid.size() != 0)
	{
		return ERROR_CONFERENCE_ALREADY_JOINED;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}
	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	m_my_user_id = myUserId;
	m_my_user_name = myUserName;

	std::map<std::string, std::string> params;
	params["conference_id"] = conferenceId;
	params["user_id"] = myUserId;
	params["user_name"] = myUserName;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_JOIN, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message TYPE_CONFERENCE_JOIN failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_start_pull_stream(std::map<std::string, std::set<std::string>> &streams)
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() == 0 || m_my_user_uuid.size() == 0)
	{
		return ERROR_CONFERENCE_NOT_JOINED;
	}

	streams.erase(m_my_user_uuid);

	if (streams.size() == 0)
	{
		return ERROR_INVALID_PARAMS;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}
	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	int count = 0;
	JsonObject streamsJson;

	std::map<std::string, std::set<std::string>>::const_iterator it = streams.begin();
	for (; it != streams.end(); it++)
	{
		std::set<std::string> ssrcSet = it->second;
		if (ssrcSet.size() == 0)
		{
			continue;
		}

		int i = 0;
		JsonObject ssrcsJson;
		std::set<std::string>::const_iterator setIT = ssrcSet.begin();
		for (; setIT != ssrcSet.end(); setIT++)
		{
			JsonObject ssrcJson;
			ssrcJson["ssrc"] = (*setIT);
			ssrcsJson[i++] = ssrcJson;
		}

		JsonObject streamJson;
		streamJson["user_uuid"] = it->first;
		streamJson["ssrcs"] = ssrcsJson;

		streamsJson[count++] = streamJson;
	}

	JsonObject params;
	params["conference_id"] = m_my_room_id;
	params["user_uuid"] = m_my_user_uuid;
	params["streams"] = streamsJson;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_PULL_STREAM, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message TYPE_CONFERENCE_PULL_STREAM failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_stop_pull_stream(std::map<std::string, std::set<std::string>> &streams)
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() == 0 || m_my_user_uuid.size() == 0)
	{
		return ERROR_CONFERENCE_NOT_JOINED;
	}

	streams.erase(m_my_user_uuid);

	if (streams.size() == 0)
	{
		return ERROR_INVALID_PARAMS;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}
	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	int count = 0;
	JsonObject streamsJson;

	std::map<std::string, std::set<std::string>>::const_iterator it = streams.begin();
	for (; it != streams.end(); it++)
	{
		std::set<std::string> ssrcSet = it->second;
		if (ssrcSet.size() == 0)
		{
			continue;
		}

		int i = 0;
		JsonObject ssrcsJson;
		std::set<std::string>::const_iterator setIT = ssrcSet.begin();
		for (; setIT != ssrcSet.end(); setIT++)
		{
			JsonObject ssrcJson;
			ssrcJson["ssrc"] = (*setIT);
			ssrcsJson[i++] = ssrcJson;
		}

		JsonObject streamJson;
		streamJson["user_uuid"] = it->first;
		streamJson["ssrcs"] = ssrcsJson;

		streamsJson[count++] = streamJson;
	}

	JsonObject params;
	params["conference_id"] = m_my_room_id;
	params["user_uuid"] = m_my_user_uuid;
	params["streams"] = streamsJson;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_STOP_PULLING, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message TYPE_CONFERENCE_STOP_PULLING failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_exit_conference()
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() == 0 || m_my_user_uuid.size() == 0)
	{
		return ERROR_CONFERENCE_NOT_JOINED;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}
	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	std::map<std::string, std::string> params;
	params["conference_id"] = m_my_room_id;
	params["user_uuid"] = m_my_user_uuid;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_EXIT, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message TYPE_CONFERENCE_EXIT failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_stop_conference()
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() == 0 || m_my_user_uuid.size() == 0)
	{
		return ERROR_CONFERENCE_NOT_JOINED;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}
	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	std::map<std::string, std::string> params;
	params["conference_id"] = m_my_room_id;
	params["user_uuid"] = m_my_user_uuid;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_STOP, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message TYPE_CONFERENCE_STOP failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_online_users()
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	if (m_my_room_id.size() == 0 || m_my_user_uuid.size() == 0)
	{
		return ERROR_CONFERENCE_NOT_JOINED;
	}

	uint64_t nowSeconds = (uint64_t)(clock() / CLOCKS_PER_SEC);
	if (m_is_signal_running && nowSeconds - m_prev_signal_time < 6)
	{
		return ERROR_REACH_MAX_API_LIMIT;
	}

	m_is_signal_running = true;
	m_prev_signal_time = nowSeconds;

	std::map<std::string, std::string> params;
	params["conference_id"] = m_my_room_id;
	params["user_uuid"] = m_my_user_uuid;

	std::string signalUUID = get_new_uuid();
	std::string signalStr = app_get_request(TYPE_CONFERENCE_ONLINE_USERS, signalUUID, params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message failed");

		m_is_signal_running = false;
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

int LiveMeetingRoom::signal_heartbeat(const std::string &myUserUUID,
									  const std::string &conferenceID)
{
	if (!m_websocket_client)
	{
		return ERROR_WEBSOCKET_NOT_INIT;
	}

	std::map<std::string, std::string> params;
	params["conference_id"] = conferenceID;
	params["user_uuid"] = myUserUUID;

	std::string signalStr = app_get_request(TYPE_CONFERENCE_HEARTBEAT, "0", params);
	bool ret = m_websocket_client->send_text(signalStr);
	if (!ret)
	{
		LOG_ERROR("send websocket message failed");
		return ERROR_WEBSOCKET_NOT_CONNECTED;
	}

	return 0;
}

void LiveMeetingRoom::add_sender_destination(const std::string &majorVideoIP,
											 uint16_t majorVideoPort_,
											 uint16_t majorVideoSequenceStart_,
											 uint32_t majorVideoTimestampStart_,
											 uint32_t majorVideoSSRC_,
											 const std::string &audioIP,
											 uint16_t audioPort_,
											 uint16_t audioSequenceStart_,
											 uint32_t audioTimestampStart_,
											 uint32_t audioSSRC_)
{
	if (!m_rtp_audio_sender || !m_rtp_video_sender)
	{
		return;
	}

	if (!m_rtp_send_initialized)
	{
		bool ret = m_rtp_audio_sender->init(audioSequenceStart_, audioTimestampStart_, audioSSRC_);
		if (!ret)
		{
			LOG_ERROR("rtp audio session initialize failed");
			return;
		}

		ret = m_rtp_video_sender->init(majorVideoSequenceStart_, majorVideoTimestampStart_, majorVideoSSRC_);
		if (!ret)
		{
			LOG_ERROR("rtp video session initialize failed");
			return;
		}

		m_rtp_send_initialized = true;
	}

	m_rtp_audio_sender->add_destination(audioIP.c_str(), audioPort_);
	m_rtp_video_sender->add_destination(majorVideoIP.c_str(), majorVideoPort_);
}

void LiveMeetingRoom::remove_sender()
{
	m_rtp_send_initialized = false;
	if (m_rtp_audio_sender)
	{
		delete m_rtp_audio_sender;
		m_rtp_audio_sender = NULL;
	}

	if (m_rtp_video_sender)
	{
		delete m_rtp_video_sender;
		m_rtp_video_sender = NULL;
	}
}

void LiveMeetingRoom::on_websocket_event(int event)
{
	if (m_signal_callback_func)
	{
		m_signal_callback_func(event, m_signal_callback_arg);
	}
}

void LiveMeetingRoom::on_websocket_message(const std::string &message)
{
	JsonObject json;
	if (!json.read_from_string(message))
	{
		LOG_ERROR("parse json error:%s", message.c_str());
		return;
	}

	//get the status code, for exampke, "200", "404"
	std::string statusCode = json["code"].as_string();
	//get the status message
	std::string statusMsg = json["msg"].as_string();
	std::string opcodeStr = json["opcode"].as_string();
	std::string requestStr = json["request"].as_string();
	CmdType opcode = (CmdType)string_to_int(opcodeStr);
	std::string messageUUID = json["uuid"].as_string();
	if (statusCode != "200")
	{
		LOG_ERROR("error: the reponse[%s] status code[%s]:[%s]", cmdtype_to_string(opcode).c_str(),
				  statusCode.c_str(), statusMsg.c_str());
		return;
	}

	JsonObject params = json["params"].as_object();
	switch (opcode)
	{
	case TYPE_CONFERENCE_CREATE: //create the conference
		on_ws_conference_create(params);
		return;

	case TYPE_CONFERENCE_JOIN: //join the conference
		on_ws_conference_join(params);
		return;

	case TYPE_CONFERENCE_NEW_JOINED: //new user has joined the conference
		on_ws_conference_new_joined(params);
		return;

	case TYPE_CONFERENCE_PULL_STREAM: //start to pull streams
		on_ws_conference_pull_stream(params);
		return;

	case TYPE_CONFERENCE_STOP_PULLING: //stop pulling streams
		on_ws_conference_stop_pulling(params);
		return;

	case TYPE_CONFERENCE_EXIT: //exit the conference
		on_ws_conference_exit(params);
		return;

	case TYPE_CONFERENCE_USER_GONE: //a new has gone out of the conference
		on_ws_conference_user_gone(params);
		return;

	case TYPE_CONFERENCE_STOP: //stop the conference
		on_ws_conference_stop(params);
		return;

	case TYPE_CONFERENCE_ONLINE_USERS: //query the online users
		on_ws_conference_online_users(params);
		return;

	case TYPE_CONFERENCE_CLOSING: //the conference is been closing
		on_ws_conference_closing(params);

	case TYPE_CONFERENCE_HEARTBEAT: //heartbeat
		on_ws_conference_heartbeat(params);
		return;

	default:
		LOG_ERROR("Unknown websocket response message");
	}
}

void LiveMeetingRoom::on_ws_conference_create(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();
	std::string userID = params["user_id"].as_string();
	std::string userName = params["user_name"].as_string();
	std::string userIP = params["user_ip"].as_string();
	std::string userUUID = params["user_uuid"].as_string();
	uint32_t videoSSRC = (uint32_t)string_to_long(params["video_ssrc"].as_string());
	uint32_t audioSSRC = (uint32_t)string_to_long(params["audio_ssrc"].as_string());
	std::string pushVideoIP = params["push_video_ip"].as_string();
	uint16_t pushVideoPort = (uint16_t)(string_to_int(params["push_video_port"].as_string()));
	std::string pushAudioIP = params["push_audio_ip"].as_string();
	uint16_t pushAudioPort = (uint16_t)(string_to_int(params["push_audio_port"].as_string()));

	LOG_DEBUG("create conference:");
	LOG_DEBUG("\tconference_id:%s", confeID.c_str());
	LOG_DEBUG("\tuser_id:%s", userID.c_str());
	LOG_DEBUG("\tuser_name:%s", userName.c_str());
	LOG_DEBUG("\tuser_ip:%s", userIP.c_str());
	LOG_DEBUG("\tuser_uuid:%s", userUUID.c_str());
	LOG_DEBUG("\tvideo_ssrc:%u", videoSSRC);
	LOG_DEBUG("\taudio_ssrc:%u", audioSSRC);
	LOG_DEBUG("\tpush_video_ip:%s", pushVideoIP.c_str());
	LOG_DEBUG("\tpush_video_port:%u", pushVideoPort);
	LOG_DEBUG("\tpush_audio_ip:%s", pushAudioIP.c_str());
	LOG_DEBUG("\tpush_audio_port:%u", pushAudioPort);

	m_my_room_id = confeID;
	m_my_ip_addr = userIP;
	m_my_user_id = userID;
	m_my_user_name = userName;
	m_my_user_uuid = userUUID;
	m_video_ssrc = videoSSRC;
	m_audio_ssrc = audioSSRC;
	m_video_push_ip = pushVideoIP;
	m_video_push_port = pushVideoPort;
	m_audio_push_ip = pushAudioIP;
	m_audio_push_port = pushAudioPort;

	add_sender_destination(m_video_push_ip, m_video_push_port, 0, 0, m_video_ssrc,
						   m_audio_push_ip, m_audio_push_port, 0, 0, m_audio_ssrc);

	//start the receive thread
	if (!m_receive_thread_running)
	{
		m_receive_thread_running = true;
#ifdef _WIN32
		m_receive_thread = std::thread(std::bind(&LiveMeetingRoom::receive_loop, this));
#else
		int ret = pthread_create(&m_receive_thread, NULL, receive_thread_func, this);
		if (ret != 0)
		{
			LOG_ERROR("Start receive thread error.");
			m_receive_thread_running = false;
		}
#endif
	}

	m_is_signal_running = false;

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_CONFERENCE_CREATED, NULL, m_event_callback_arg);
	}
}

void LiveMeetingRoom::on_ws_conference_join(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();

	std::string userID = params["user_id"].as_string();
	std::string userName = params["user_name"].as_string();
	std::string userIP = params["user_ip"].as_string();
	std::string userUUID = params["user_uuid"].as_string();
	uint32_t videoSSRC = (uint32_t)string_to_long(params["video_ssrc"].as_string());
	uint32_t audioSSRC = (uint32_t)string_to_long(params["audio_ssrc"].as_string());
	std::string pushVideoIP = params["push_video_ip"].as_string();
	uint16_t pushVideoPort = (uint16_t)(string_to_int(params["push_video_port"].as_string()));
	std::string pushAudioIP = params["push_audio_ip"].as_string();
	uint16_t pushAudioPort = (uint16_t)(string_to_int(params["push_audio_port"].as_string()));

	LOG_DEBUG("conference join:");
	LOG_DEBUG("\tuser_id:%s", userID.c_str());
	LOG_DEBUG("\tuser_name:%s", userName.c_str());
	LOG_DEBUG("\tuser_ip:%s", userIP.c_str());
	LOG_DEBUG("\tuser_uuid:%s", userUUID.c_str());
	LOG_DEBUG("\tvideo_ssrc:%u", videoSSRC);
	LOG_DEBUG("\taudio_ssrc:%u", audioSSRC);
	LOG_DEBUG("\tpush_video_ip:%s", pushVideoIP.c_str());
	LOG_DEBUG("\tpush_video_port:%u", pushVideoPort);
	LOG_DEBUG("\tpush_audio_ip:%s", pushAudioIP.c_str());
	LOG_DEBUG("\tpush_audio_port:%u", pushAudioPort);

	if (userUUID == m_my_user_uuid || userID == m_my_user_id)
	{
		m_my_room_id = confeID;
		m_my_ip_addr = userIP;
		m_my_user_id = userID;
		m_my_user_name = userName;
		m_my_user_uuid = userUUID;
		m_video_ssrc = videoSSRC;
		m_audio_ssrc = audioSSRC;
		m_video_push_ip = pushVideoIP;
		m_video_push_port = pushVideoPort;
		m_audio_push_ip = pushAudioIP;
		m_audio_push_port = pushAudioPort;

		add_sender_destination(m_video_push_ip, m_video_push_port, 0, 0, m_video_ssrc,
							   m_audio_push_ip, m_audio_push_port, 0, 0, m_audio_ssrc);

		//start the receive thread
		if (!m_receive_thread_running)
		{
			m_receive_thread_running = true;
#ifdef _WIN32
			m_receive_thread = std::thread(std::bind(&LiveMeetingRoom::receive_loop, this));
#else
			int ret = pthread_create(&m_receive_thread, NULL, receive_thread_func, this);
			if (ret != 0)
			{
				LOG_ERROR("Start receive thread error.");
				m_receive_thread_running = false;
			}
#endif
		}
	}

	m_is_signal_running = false;
	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_CONFERENCE_JOINED, NULL, m_event_callback_arg);
	}
}

void LiveMeetingRoom::on_ws_conference_new_joined(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();

	std::string userID = params["user_id"].as_string();
	std::string userName = params["user_name"].as_string();
	std::string userIP = params["user_ip"].as_string();
	std::string userUUID = params["user_uuid"].as_string();
	uint32_t videoSSRC = (uint32_t)string_to_long(params["video_ssrc"].as_string());
	uint32_t audioSSRC = (uint32_t)string_to_long(params["audio_ssrc"].as_string());

	LOG_DEBUG("conference new joined:");
	LOG_DEBUG("\tuser_id:%s", userID.c_str());
	LOG_DEBUG("\tuser_name:%s", userName.c_str());
	LOG_DEBUG("\tuser_ip:%s", userIP.c_str());
	LOG_DEBUG("\tuser_uuid:%s", userUUID.c_str());
	LOG_DEBUG("\tvideo_ssrc:%u", videoSSRC);
	LOG_DEBUG("\taudio_ssrc:%u", audioSSRC);

#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_receive_mutex);
#else
	pthread_mutex_lock(&m_receive_mutex);
#endif
	std::map<std::string, RoomUser *>::iterator it = m_other_users_map.find(userUUID);
	if (it == m_other_users_map.end())
	{
		RoomUser *roomUser = RoomUser::create_user();
		roomUser->set_user_id(userID);
		roomUser->set_user_name(userName);
		roomUser->set_user_ip_addr(userIP);
		roomUser->set_user_uuid(userUUID);
		roomUser->set_video_ssrc(videoSSRC);
		roomUser->set_audio_ssrc(audioSSRC);
		roomUser->set_aac_receive_callback(m_aac_callback, m_aac_callback_arg);
		roomUser->set_h264_receive_callback(m_h264_callback, m_h264_callback_arg);
		roomUser->set_pinhole_uuid(m_my_user_uuid);

		m_other_users_map[userUUID] = roomUser;
	}
#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_receive_mutex);
#endif

	struct OnlineUser user;
	user.userID = userID;
	user.userName = userName;
	user.userIP = userIP;
	user.userUUID = userUUID;
	user.videoSSRC = videoSSRC;
	user.audioSSRC = audioSSRC;

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_OTHER_USER_JOINED, &user, m_event_callback_arg);
	}
}

void LiveMeetingRoom::on_ws_conference_pull_stream(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();
	std::string userUUID = params["user_uuid"].as_string();

	JsonObject streamsJson = params["streams"].as_object();
	int count = streamsJson.array_size();
	for (int i = 0; i < count; i++)
	{
		JsonObject streamJson = streamsJson[i].as_object();
		std::string streamUserUUID = streamJson["user_uuid"].as_string(); //user uuid

#ifdef _WIN32
		m_receive_mutex.lock();
#else
		pthread_mutex_lock(&m_receive_mutex);
#endif
		std::map<std::string, RoomUser *>::iterator it = m_other_users_map.find(streamUserUUID);
		if (it == m_other_users_map.end())
		{
#ifdef _WIN32
			m_receive_mutex.unlock();
#else
			pthread_mutex_unlock(&m_receive_mutex);
#endif
			continue;
		}
		RoomUser *roomUser = it->second;
#ifdef _WIN32
		m_receive_mutex.unlock();
#else
		pthread_mutex_unlock(&m_receive_mutex);
#endif

		JsonObject ssrcsJson = streamJson["ssrcs"].as_object();
		int ssrcCount = ssrcsJson.array_size();
		for (int j = 0; j < ssrcCount; j++)
		{
			JsonObject ssrcJson = ssrcsJson[j].as_object();
			std::string ssrcStr = ssrcJson["ssrc"].as_string();
			std::string pinholeIP = ssrcJson["ip"].as_string(); //ip
			std::string portStr = ssrcJson["port"].as_string();

			uint32_t ssrc = (uint32_t)string_to_long(ssrcStr);		 //ssrc
			uint16_t pinholePort = (uint16_t)string_to_int(portStr); //port

			roomUser->initialize(pinholeIP.c_str(), pinholePort, ssrc);
		}
	}

	m_is_signal_running = false;
}

void LiveMeetingRoom::on_ws_conference_stop_pulling(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();
	std::string userUUID = params["user_uuid"].as_string();

	m_is_signal_running = false;
}

void LiveMeetingRoom::on_ws_conference_exit(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();
	std::string userUUID = params["user_uuid"].as_string();

	stop_receive();
	remove_sender();

#ifdef _WIN32
	m_receive_mutex.lock();
#else
	pthread_mutex_lock(&m_receive_mutex);
#endif
	std::map<std::string, RoomUser *>::iterator it = m_other_users_map.begin();
	for (; it != m_other_users_map.end(); it++)
	{
		RoomUser *roomUser = it->second;
		delete roomUser;
	}
	m_other_users_map.clear();
#ifdef _WIN32
	m_receive_mutex.unlock();
#else
	pthread_mutex_unlock(&m_receive_mutex);
#endif

	m_my_room_id = "";
	m_my_ip_addr = "";
	m_my_user_id = "";
	m_my_user_name = "";
	m_my_user_uuid = "";
	m_video_push_ip = "";
	m_audio_push_ip = "";
	m_video_push_port = 0;
	m_audio_push_port = 0;
	m_video_ssrc = 0;
	m_audio_ssrc = 0;

	m_is_signal_running = false;

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_CONFERENCE_EXIT, &userUUID, m_event_callback_arg);
	}
}

void LiveMeetingRoom::on_ws_conference_user_gone(JsonObject &params)
{
	std::string confeID = params["conference_id"].as_string();
	std::string userUUID = params["user_uuid"].as_string();

	std::string userName;
#ifdef _WIN32
	m_receive_mutex.lock();
#else
	pthread_mutex_lock(&m_receive_mutex);
#endif
	std::map<std::string, RoomUser *>::iterator it = m_other_users_map.find(userUUID);
	if (it != m_other_users_map.end())
	{
		RoomUser *roomUser = it->second;
		userName = roomUser->get_user_name();
		delete roomUser;
		m_other_users_map.erase(it);
	}
#ifdef _WIN32
	m_receive_mutex.unlock();
#else
	pthread_mutex_unlock(&m_receive_mutex);
#endif

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_USER_GONE_OUT, &userUUID, m_event_callback_arg);
	}
}

void LiveMeetingRoom::on_ws_conference_online_users(JsonObject &params)
{
	m_is_signal_running = false;

	std::string confeID = params["conference_id"].as_string();

	std::vector<struct OnlineUser *> userVec;

	//the online users in the conference
	JsonObject onlineUsersJson = params["online_users"].as_object();
	int usersCount = onlineUsersJson.array_size();
	if (usersCount != -1)
	{
#ifdef _WIN32
		m_receive_mutex.lock();
#else
		pthread_mutex_lock(&m_receive_mutex);
#endif
		for (int i = 0; i < usersCount; i++)
		{
			JsonObject onlineUserJson = onlineUsersJson[i].as_object();

			std::string userID = onlineUserJson["user_id"].as_string();
			std::string userName = onlineUserJson["user_name"].as_string();
			std::string userIP = onlineUserJson["user_ip"].as_string();
			std::string userUUID = onlineUserJson["user_uuid"].as_string();
			uint32_t videoSSRC = (uint32_t)string_to_long(onlineUserJson["video_ssrc"].as_string());
			uint32_t audioSSRC = (uint32_t)string_to_long(onlineUserJson["audio_ssrc"].as_string());

			struct OnlineUser *onlineUser = new OnlineUser();
			if (!onlineUser)
			{
				continue;
			}

			onlineUser->userID = userID;
			onlineUser->userName = userName;
			onlineUser->userIP = userIP;
			onlineUser->userUUID = userUUID;
			onlineUser->videoSSRC = videoSSRC;
			onlineUser->audioSSRC = audioSSRC;

			userVec.push_back(onlineUser);

			std::map<std::string, RoomUser *>::iterator it = m_other_users_map.find(userUUID);
			if (it == m_other_users_map.end())
			{
				RoomUser *roomUser = RoomUser::create_user();
				roomUser->set_user_id(userID);
				roomUser->set_user_name(userName);
				roomUser->set_user_ip_addr(userIP);
				roomUser->set_user_uuid(userUUID);
				roomUser->set_video_ssrc(videoSSRC);
				roomUser->set_audio_ssrc(audioSSRC);
				roomUser->set_aac_receive_callback(m_aac_callback, m_aac_callback_arg);
				roomUser->set_h264_receive_callback(m_h264_callback, m_h264_callback_arg);
				roomUser->set_pinhole_uuid(m_my_user_uuid);

				m_other_users_map[userUUID] = roomUser;
			}
		}
#ifdef _WIN32
		m_receive_mutex.unlock();
#else
		pthread_mutex_unlock(&m_receive_mutex);
#endif
	}

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_ONLINE_USERS, &userVec, m_event_callback_arg);
	}

	std::vector<struct OnlineUser *>::iterator it = userVec.begin();
	for (; it != userVec.end(); it++)
	{
		delete (*it);
	}
}

void LiveMeetingRoom::on_ws_conference_heartbeat(JsonObject &params)
{
}

void LiveMeetingRoom::on_ws_conference_stop(JsonObject &params)
{
	std::string conferenceId = params["conference_id"].as_string();

	stop_receive();
	remove_sender();

#ifdef _WIN32
	m_receive_mutex.lock();
#else
	pthread_mutex_lock(&m_receive_mutex);
#endif
	std::map<std::string, RoomUser *>::iterator it = m_other_users_map.begin();
	for (; it != m_other_users_map.end(); it++)
	{
		RoomUser *roomUser = it->second;
		delete roomUser;
	}
	m_other_users_map.clear();
#ifdef _WIN32
	m_receive_mutex.unlock();
#else
	pthread_mutex_unlock(&m_receive_mutex);
#endif

	m_my_room_id = "";
	m_my_ip_addr = "";
	m_my_user_id = "";
	m_my_user_name = "";
	m_my_user_uuid = "";
	m_video_push_ip = "";
	m_audio_push_ip = "";
	m_video_push_port = 0;
	m_audio_push_port = 0;
	m_video_ssrc = 0;
	m_audio_ssrc = 0;

	m_is_signal_running = false;

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_CONFERENCE_STOPPED, &conferenceId, m_event_callback_arg);
	}
}

void LiveMeetingRoom::on_ws_conference_closing(JsonObject &params)
{
	std::string conferenceId = params["conference_id"].as_string();
	LOG_DEBUG("conference is closing");
	LOG_DEBUG("\tconference_id:%s", conferenceId.c_str());

	stop_receive();
	remove_sender();

#ifdef _WIN32
	m_receive_mutex.lock();
#else
	pthread_mutex_lock(&m_receive_mutex);
#endif
	std::map<std::string, RoomUser *>::iterator it = m_other_users_map.begin();
	for (; it != m_other_users_map.end(); it++)
	{
		RoomUser *roomUser = it->second;
		delete roomUser;
	}
	m_other_users_map.clear();
#ifdef _WIN32
	m_receive_mutex.unlock();
#else
	pthread_mutex_unlock(&m_receive_mutex);
#endif

	m_my_room_id = "";
	m_my_ip_addr = "";
	m_my_user_id = "";
	m_my_user_name = "";
	m_my_user_uuid = "";
	m_video_push_ip = "";
	m_audio_push_ip = "";
	m_video_push_port = 0;
	m_audio_push_port = 0;
	m_video_ssrc = 0;
	m_audio_ssrc = 0;

	m_is_signal_running = false;

	if (m_event_callback_func)
	{
		m_event_callback_func(EVENT_CONFERENCE_CLOSING, &conferenceId, m_event_callback_arg);
	}
}