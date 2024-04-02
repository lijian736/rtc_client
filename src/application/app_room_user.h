#ifndef _H_APP_ROOM_USER_H_
#define _H_APP_ROOM_USER_H_

#include <string>
#include <stdint.h>

#include "rtp_session_receiver.h"
#include "rtp_h264_frame_assembler.h"
#include "codec_utils.h"

//the H.264 data receive callback function
typedef void (*OnH264ReceiveCallback)(std::string &uuid, uint8_t *data, int length, uint32_t ssrc, uint32_t timestamp, void* userArg);

//the AAC data receive callback function
typedef void (*OnAACReceiveCallback)(std::string &uuid, uint8_t *data, int length, uint32_t ssrc, uint32_t timestamp, void* userArg);

//the user in the meeting room
class RoomUser
{
private:
	RoomUser();

public:
	//the destructor will be blocked until the inner thread terminates
	virtual ~RoomUser();

	/**
	 * @brief Create a meeting room user
	 * @return RoomUser* if create successful, return the user pointer, otherwise return NULL
	 */
	static RoomUser *create_user();

	/**
	* @brief initialize the user
	* @param pinholeIP - the pinhole ip address
	* @param pinholePort - the pinhole port
	* @param ssrc - the stream ssrc
	* @return true - successful, false - fail
	*/
	bool initialize(const char *pinholeIP,
					const uint16_t &pinholePort,
					uint32_t ssrc);

	/**
	 * @brief the receive NAT pinhole
	 */
	void nat_pinhole();

	/**
	 * @brief Get the major video port
	 * 
	 * @return uint16_t 
	 */
	uint16_t get_major_port() const { return this->m_bind_major_port; }

	/**
	 * @brief Get the audio port
	 * 
	 * @return uint16_t 
	 */
	uint16_t get_audio_port() const { return this->m_bind_audio_port; }

	/**
	 * @brief Set the h264 receive callback function
	 * @param func -- the H.264 data receive function
	 * @param arg -- the user argument
	 */
	void set_h264_receive_callback(OnH264ReceiveCallback func, void* arg);

	/**
	 * @brief Set the aac receive callback function
	 * @param func -- the aac data receive function
	 * @param arg -- the user argument
	 */
	void set_aac_receive_callback(OnAACReceiveCallback func, void* arg);

	/**
	 * @brief receive video
	 */
	void receive_video();

	/**
	 * @brief receive audio
	 * 
	 */
	void receive_audio();

	RoomUser *set_user_id(const std::string &userID);
	RoomUser *set_user_name(const std::string &userName);
	RoomUser *set_user_uuid(const std::string &userUUID);
	RoomUser *set_user_ip_addr(const std::string &ipAddr);
	RoomUser *set_video_ssrc(uint32_t ssrc);
	RoomUser *set_audio_ssrc(uint32_t ssrc);
	RoomUser *set_pinhole_uuid(const std::string& pinholeUUID);

	std::string get_user_id() const;
	std::string get_user_name() const;
	std::string get_user_uuid() const;
	std::string get_user_ip_addr() const;
	std::string get_pinhole_uuid() const;
	uint32_t get_video_ssrc() const;
	uint32_t get_audio_ssrc() const;

private:
	/**
	* @brief initialize the user
	* @param pinholeIP - the pinhole ip address
	* @param pinholePort - the pinhole port
	* @return true - successful, false - fail
	*/
	bool initialize_video(const char *pinholeIP,
						  const uint16_t &pinholePort);

	/**
	* @brief initialize the user
	* @param pinholeIP - the pinhole ip address
	* @param pinholePort - the pinhole port
	* @return true - successful, false - fail
	*/
	bool initialize_audio(const char *pinholeIP,
						  const uint16_t &pinholePort);
	/**
	* @brief get an availabel port for the user's rtp session
	* @return the port, if no port is available, then return 0
	*/
	static uint16_t get_available_port();

private:
	bool m_video_initialize;
	bool m_audio_initialize;

	//the major video rtp bind port
	uint16_t m_bind_major_port;
	//the audio rtp bind port
	uint16_t m_bind_audio_port;

	//the major video receiver
	RTPSessionReceiver *m_major_receiver;
	//the audio receiver
	RTPSessionReceiver *m_audio_receiver;

	//the h264 rtp frame assembler
	RTPH264FrameAssembler *m_h264_frame_assembler;

	//the H.264 rtp receive buffer
	uint8_t *m_h264_rtp_buffer;
	//the H.264 rtp buffer used length
	size_t m_h264_rtp_buffer_used_len;

	OnH264ReceiveCallback m_h264_callback;
	void* m_h264_callback_arg;

	OnAACReceiveCallback m_aac_callback;
	void* m_aac_callback_arg;

	//the user id
	std::string m_user_id;
	//the user name
	std::string m_user_name;
	//the user uuid
	std::string m_user_uuid;
	//the user ip address
	std::string m_user_ip_addr;
	//the video ssrc
	uint32_t m_video_ssrc;
	//the audio ssrc
	uint32_t m_audio_ssrc;
	//the pinhole uuid
	std::string m_pinhole_uuid;

	//the pinghole message
	std::string m_pinhole_msg;
};

#endif