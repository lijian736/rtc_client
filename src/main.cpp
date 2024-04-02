#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <new>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>

#include "common_utils.h"
#include "common_json.h"
#include "common_logger.h"

#include "common_port_manager.h"
#include "ws_client.h"
#include "app_util.h"
#include "app_meeting_room.h"
#include "app_event.h"

//日志对象
AppLogger *g_pLogger = NULL;
//日志等级
int g_log_level = LOG_LEVEL_VERBOSE;

//视频会议对象
LiveMeetingRoom *g_room = NULL;

//是否已经和服务器连接
bool g_room_connected;
//是否已经加入视频会议
bool g_in_room;

//发送H.264数据
// g_room->send_h264_data(data, length);

//发送AAC数据
// g_room->send_aac_data(data, length);

//H.264数据的接收回调函数
//参数说明：
//uuid -- 用户的uuid
//data -- 数据指针
//length -- 数据长度
//ssrc -- H.264视频流的ssrc
//timestamp -- 时间戳
//userArg -- 用户传入的自定义参数
void h264_receive_callback(std::string &uuid, uint8_t *data, int length,
						   uint32_t ssrc, uint32_t timestamp, void *userArg)
{
}

//AAC数据的接收回调函数
//参数说明：
//uuid -- 用户的uuid
//data -- 数据指针
//length -- 数据长度
//ssrc -- AAC音频流的ssrc
//timestamp -- 时间戳
//userArg -- 用户传入的自定义参数
void aac_receive_callback(std::string &uuid, uint8_t *data, int length,
						  uint32_t ssrc, uint32_t timestamp, void *userArg)
{
}

//视频会议WebSocket连接回调函数
//参数说明：
//event -- WebSocket事件类型
//userArg -- 用户传入的自定义参数
void conference_signal_callback(int event, void *userArg)
{
	if (event == MG_EV_WS_OPEN) // 和服务器的websocket连接建立
	{
		g_room_connected = true;
	}
	else if (event == MG_EV_CLOSE) // 和服务器的websocket连接关闭
	{
		g_room_connected = false;
	}
}

//视频会议的事件回调函数，视频会议的主要逻辑处理函数。包括创建会议，加入会议，推出会议，停止会议
//请求其他与会者的码流，停止其他与会者的码流等
//event -- 视频会议事件
//data -- 附加数据
//userArg -- 用户传入的自定义参数
void conference_event_callback(int event, void *data, void *userArg)
{
	switch (event)
	{
	case EVENT_CONFERENCE_CREATED:   //创建会议,视频会议的创建者会收到该回调函数
		{
			//视频会议号
			std::string roomID = g_room->get_room_id();
			//本机用户名
			std::string userName = g_room->get_my_user_name();
			//本机的因特网IP地址
			std::string ip = g_room->get_my_ip_addr();
			//本机的用户UUID，该uuid唯一表示视频会议中的用户
			std::string my_user_uuid = g_room->get_my_user_uuid();

			//已加入会议。会议的创建者，默认加入会议
			g_in_room = true;
		}
		return;

	case EVENT_CONFERENCE_JOINED:   //加入会议，加入视频会议的用户（非创建者）会收到该回调函数
		{
			//已加入会议
			g_in_room = true;
			//本机的用户UUID，该uuid唯一表示视频会议中的用户
			std::string my_user_uuid = g_room->get_my_user_uuid();
			//视频会议号
			std::string roomID = g_room->get_room_id();

			//请求当前会议中在线用户信息
			g_room->signal_online_users();
		}
		return;

	case EVENT_OTHER_USER_JOINED:   //其他用户新加入会议。当有新用户加入会议时，已加入会议的成员会收到该回调函数
		{
			struct OnlineUser *user = (struct OnlineUser *)data;
			if (user)
			{
				//新加入用户的用户ID
				std::string userID = user->userID;
				//新加入用户的用户名称
				std::string userName = user->userName;
				//新加入用户的用户IP
				std::string userIP = user->userIP;
				//新加入用户的用户UUID
				std::string userUUID = user->userUUID;
				//新加入用户的视频SSRC(视频流的唯一标识)
				uint32_t videoSSRC = user->videoSSRC;
				//新加入用户的音频SSRC(音频流的唯一标识)
				uint32_t audioSSRC = user->audioSSRC;

				std::set<std::string> ssrcs;
				ssrcs.insert(common_to_string(user->videoSSRC));
				ssrcs.insert(common_to_string(user->audioSSRC));

				std::map<std::string, std::set<std::string>> streams;
				streams[user->userUUID] = ssrcs;
				
				//请求新加入用户的视频流和音频流(可以在其他地方请求)
				g_room->signal_start_pull_stream(streams);
			}
		}
		return;

	case EVENT_ONLINE_USERS:  //当前会议的在线人数, 调用g_room->signal_online_users()成功后的回调函数
		{
			std::map<std::string, std::set<std::string>> streams;

			std::vector<struct OnlineUser *> *userVec = (std::vector<struct OnlineUser *> *)data;
			std::vector<struct OnlineUser *>::iterator it = userVec->begin();
			for (; it != userVec->end(); it++)
			{
				struct OnlineUser *user = *it;
				//用户ID
				std::string userID = user->userID;
				//用户名称
				std::string userName = user->userName;
				//用户IP地址
				std::string userIP = user->userIP;
				//用户UUID
				std::string userUUID = user->userUUID;
				//用户的视频SSRC(唯一标识该视频流)
				uint32_t videoSSRC = user->videoSSRC;
				//用户的音频SSRC(唯一标识该音频流)
				uint32_t audioSSRC = user->audioSSRC;

				std::set<std::string> ssrcs;
				ssrcs.insert(common_to_string(user->videoSSRC));
				ssrcs.insert(common_to_string(user->audioSSRC));

				streams[user->userUUID] = ssrcs;
			}

			//请求当前在线用户的音视频流
			g_room->signal_start_pull_stream(streams);
		}
		return;

	case EVENT_CONFERENCE_EXIT:   //本机用户退出视频会议的回调函数
		{
			//用户的UUID
			std::string *userUUID = (std::string *)data;
			LOG_DEBUG("conference eixt, user uuid:%s", (*userUUID).c_str());
		}
		return;

	case EVENT_USER_GONE_OUT:   //其他用户退出视频会议的回调函数
		{
			//用户的UUID
			std::string *userUUID = (std::string *)data;
			LOG_DEBUG("user gone out of conference, user uuid:%s", (*userUUID).c_str());
		}
		return;

	case EVENT_CONFERENCE_STOPPED:  //视频会议停止的回调函数。当视频会议的创建者停止视频会议时，收到此回调函数
		{
			//会议的ID
			std::string *conferenceID = (std::string *)data;
			LOG_DEBUG("conference eixt, conference id:%s", (*conferenceID).c_str());
		}
		return;

	case EVENT_CONFERENCE_CLOSING:   //视频会议关闭的回调函数。当视频会议的创建者停止视频会议时，其他用户（非创建者）收到此回调函数
		{
			//会议的ID
			std::string *conferenceID = (std::string *)data;
			LOG_DEBUG("conference closing, conference id:%s", (*conferenceID).c_str());
		}
		return;
	}
}

int main(int argc, char *argv[])
{
	bool bRet;
	int joinRet;
	AppLogger *logger;

	//创建日志文件目录
	if (!check_path_exists("rtc_client"))
	{
		bRet = create_directory_recursively("rtc_client");
		std::cout << "the directory 'rtc_client' NOT exists, create returns:" << bRet << std::endl;
	}
	else
	{
		std::cout << "the directory 'rtc_client' exists" << std::endl;
	}

	//初始化日志对象
	logger = new AppLogger("rtc_client");
	if (logger)
	{
		bRet = logger->initialize("rtc_client", false, 1024 * 10, 10);
		std::cout << "AppLogger::initialize(): " << bRet << std::endl;

		g_pLogger = logger;
	}
	else
	{
		goto exitFlag;
	}

	//初始化端口信息,客户端在视频会议过程中会使用该端口进行视频通信
	PortManager::get_instance()->initialize_udp_ports(30000, 30100);

	LOG_INFO("Program begins..................................");

	//获取视频会议对象,该对象为全局对象
	g_room = LiveMeetingRoom::get_instance();
	//初始化视频会议对象，参数为:服务器地址和端口号
	bRet = g_room->initialize("192.168.1.100", 23579);
	if (bRet)
	{
		LOG_INFO("LiveMeetingRoom initialize returns:%s", bRet ? "true" : "false");
	}
	else
	{
		goto exitFlag;
	}

	//设置视频会议的各种回调函数
	//设置视频会议事件回调函数
	g_room->set_event_callback_func(conference_event_callback, NULL);
	//设置视频会议信令WebSocket回调函数
	g_room->set_signal_callback_func(conference_signal_callback, NULL);
	//设置接收H.264的回调函数
	g_room->set_h264_receive_callback(h264_receive_callback, NULL);
	//设置接收AAC的回调函数
	g_room->set_aac_receive_callback(aac_receive_callback, NULL);

	//以下注释的为视频会议的操作
	//创建视频会议
	// g_room->signal_create_conference("id_67891011", "李四");

	//加入视频会议
	// joinRet = g_room->signal_join_conference("100000000", "id_67891011", "张三");
	//if (joinRet < 0)
	//{
	//	std::cout << "join failed: " << joinRet << std::endl;
	//}

	//请求视频会议的与会者的音视频流
	//视频流或者音频流的SSRC，放在集合std::set中
	//std::set<std::string> ssrcSet;
	//ssrcSet.insert("1");
	//ssrcSet.insert("0");
	//该视频流或者音频流属于的用户
	//std::map<std::string, std::set<std::string>> ssrcsMap;
	//key:用户的UUID， value:用户视频流或者音频流的std::set
	//ssrcsMap["df23e45a-5efd-46ef-b3c6-05f96fcf34e2"] = ssrcSet;
	//g_room->signal_stop_pull_stream(ssrcsMap);
	
	//视频会议的非创建者， 退出视频会议，
	//g_room->signal_exit_conference();

	//视频会议的创建者， 停止视频会议
	//g_room->signal_stop_conference();

exitFlag:

	LOG_INFO("Program ends......................................");
	if (g_room)
	{
		//反初始化，回收资源
		g_room->un_initialize();
	}

	if (g_pLogger)
	{
		//日志对象反初始化，回收资源
		g_pLogger->uninitialize();
		delete g_pLogger;
	}

	return 0;
}