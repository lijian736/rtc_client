#include "app_room_user.h"

#include <string.h>

#include "common_logger.h"
#include "common_port_manager.h"
#include "common_json.h"
#include "codec_utils.h"

static const int RTP_H264_RECV_BUFFER_SIZE = 1024 * 1024;

RoomUser::RoomUser()
{
	m_video_initialize = false;
	m_audio_initialize = false;
	m_bind_major_port = 0;
	m_bind_audio_port = 0;
	m_major_receiver = NULL;
	m_audio_receiver = NULL;

	m_h264_frame_assembler = NULL;

	m_h264_rtp_buffer = NULL;
	m_h264_rtp_buffer_used_len = 0;

	m_h264_callback = NULL;
	m_h264_callback_arg = NULL;

	m_aac_callback = NULL;
	m_aac_callback_arg = NULL;
}

RoomUser::~RoomUser()
{
	if (m_major_receiver)
	{
		delete m_major_receiver;
		m_major_receiver = NULL;
	}

	if (m_audio_receiver)
	{
		delete m_audio_receiver;
		m_audio_receiver = NULL;
	}

	if (m_h264_frame_assembler)
	{
		delete m_h264_frame_assembler;
		m_h264_frame_assembler = NULL;
	}

	if (m_h264_rtp_buffer)
	{
		delete[] m_h264_rtp_buffer;
	}
}

bool RoomUser::initialize(const char *pinholeIP,
						  const uint16_t &pinholePort,
						  uint32_t ssrc)
{
	if (m_video_ssrc == ssrc)
	{
		return initialize_video(pinholeIP, pinholePort);
	}
	else if (m_audio_ssrc == ssrc)
	{
		return initialize_audio(pinholeIP, pinholePort);
	}
	else
	{
		return false;
	}
}

bool RoomUser::initialize_video(const char *pinholeIP,
								const uint16_t &pinholePort)
{
	bool ret;
	if (m_video_initialize)
	{
		return true;
	}

	uint16_t majorPort = get_available_port();
	if (majorPort == 0)
	{
		LOG_ERROR("No available ports for rtp receiver");
		return false;
	}

	LOG_DEBUG("receive video port:%d", majorPort);
	this->m_bind_major_port = majorPort;
	RTPTransParamsV4 rtpVideoParams;
	m_major_receiver = new (std::nothrow) RTPSessionReceiver();
	if (!m_major_receiver)
	{
		LOG_ERROR("Create RTPSessionReceiver error.");
		goto exitFlag;
	}

	rtpVideoParams.bindIP = 0;
	rtpVideoParams.bindPort = this->m_bind_major_port;
	ret = m_major_receiver->init(true, &rtpVideoParams, pinholeIP, pinholePort);
	if (!ret)
	{
		LOG_ERROR("init RTPSessionReceiver error.");
		goto exitFlag;
	}

	m_h264_frame_assembler = new (std::nothrow) RTPH264FrameAssembler();
	if (!m_h264_frame_assembler)
	{
		LOG_ERROR("Create RTPH264FrameAssembler error.");
		goto exitFlag;
	}

	if (!m_h264_frame_assembler->initialize())
	{
		LOG_ERROR("Create RTPH264FrameAssembler error.");
		goto exitFlag;
	}

	m_h264_rtp_buffer = new uint8_t[RTP_H264_RECV_BUFFER_SIZE];
	m_h264_rtp_buffer_used_len = 0;
	if (!m_h264_rtp_buffer)
	{
		LOG_ERROR("Create H.264 RTP receive buffer error.");
		goto exitFlag;
	}

	m_video_initialize = true;
	return true;

exitFlag:

	if (m_major_receiver)
	{
		delete m_major_receiver;
		m_major_receiver = NULL;
	}

	if (m_h264_frame_assembler)
	{
		delete m_h264_frame_assembler;
		m_h264_frame_assembler = NULL;
	}

	if (m_h264_rtp_buffer)
	{
		delete[] m_h264_rtp_buffer;
		m_h264_rtp_buffer = NULL;
	}

	return false;
}

bool RoomUser::initialize_audio(const char *pinholeIP,
								const uint16_t &pinholePort)
{
	bool ret;
	if (m_audio_initialize)
	{
		return true;
	}

	uint16_t audioPort = get_available_port();
	if (audioPort == 0)
	{
		LOG_ERROR("No available ports for rtp receiver");
		return false;
	}

	this->m_bind_audio_port = audioPort;
	RTPTransParamsV4 rtpAudioParams;

	m_audio_receiver = new (std::nothrow) RTPSessionReceiver();
	if (!m_audio_receiver)
	{
		LOG_ERROR("Create RTPSessionReceiver error.");
		goto exitFlag;
	}

	rtpAudioParams.bindIP = 0;
	rtpAudioParams.bindPort = this->m_bind_audio_port;
	ret = m_audio_receiver->init(true, &rtpAudioParams, pinholeIP, pinholePort);
	if (!ret)
	{
		LOG_ERROR("init RTPSessionReceiver error.");
		goto exitFlag;
	}

	m_audio_initialize = true;
	return true;

exitFlag:

	if (m_audio_receiver)
	{
		delete m_audio_receiver;
		m_audio_receiver = NULL;
	}

	return false;
}

uint16_t RoomUser::get_available_port()
{
	uint16_t port;
	bool ret = PortManager::get_instance()->get_udp_port(port);

	return ret ? port : 0;
}

void RoomUser::set_h264_receive_callback(OnH264ReceiveCallback func, void* arg)
{
	m_h264_callback = func;
	m_h264_callback_arg = arg;
}

void RoomUser::set_aac_receive_callback(OnAACReceiveCallback func, void* arg)
{
	m_aac_callback = func;
	m_aac_callback_arg = arg;
}

void RoomUser::receive_video()
{
	if(!m_major_receiver)
	{
		return;
	}

	RTPPacket *rtp = m_major_receiver->receive_rtp_packet(5, 0);
	if (rtp)
	{
		if (m_h264_frame_assembler->push_packet(rtp))
		{
			uint8_t *data = m_h264_frame_assembler->get_frame_data();
			size_t len = m_h264_frame_assembler->get_frame_length();

			if (m_h264_rtp_buffer_used_len + len >= RTP_H264_RECV_BUFFER_SIZE)
			{
				m_h264_rtp_buffer_used_len = 0;
			}

			memcpy(m_h264_rtp_buffer + m_h264_rtp_buffer_used_len, data, len);
			m_h264_rtp_buffer_used_len += len;

			if (rtp->has_marker())
			{
				//the callback
				if (m_h264_callback)
				{
					uint32_t ts = rtp->get_timestamp();
					uint32_t ssrc = rtp->get_ssrc();
					m_h264_callback(m_user_uuid, m_h264_rtp_buffer, m_h264_rtp_buffer_used_len, ssrc, ts, m_h264_callback_arg);
				}

				m_h264_rtp_buffer_used_len = 0;
			}
		}

		m_major_receiver->end_receive_rtp_packet(rtp);
	}
}

void RoomUser::receive_audio()
{
	if(!m_audio_receiver)
	{
		return;
	}

	RTPPacket *rtp = m_audio_receiver->receive_rtp_packet(5, 0);
	if (rtp)
	{
		if (m_aac_callback)
		{
			uint32_t ts = rtp->get_timestamp();
			uint32_t ssrc = rtp->get_ssrc();
			m_aac_callback(m_user_uuid, rtp->get_payload(), (int)rtp->get_payload_length(), ssrc, ts, m_aac_callback_arg);
		}
		m_audio_receiver->end_receive_rtp_packet(rtp);
	}
}

RoomUser *RoomUser::set_user_id(const std::string &userID)
{
	this->m_user_id = userID;
	return this;
}

RoomUser *RoomUser::set_user_name(const std::string &userName)
{
	this->m_user_name = userName;
	return this;
}

RoomUser *RoomUser::set_user_uuid(const std::string &userUUID)
{
	this->m_user_uuid = userUUID;
	return this;
}

RoomUser *RoomUser::set_user_ip_addr(const std::string &ipAddr)
{
	this->m_user_ip_addr = ipAddr;
	return this;
}

RoomUser *RoomUser::set_video_ssrc(uint32_t ssrc)
{
	this->m_video_ssrc = ssrc;
	return this;
}

RoomUser *RoomUser::set_audio_ssrc(uint32_t ssrc)
{
	this->m_audio_ssrc = ssrc;
	return this;
}

RoomUser *RoomUser::set_pinhole_uuid(const std::string &pinholeUUID)
{
	this->m_pinhole_uuid = pinholeUUID;

	JsonObject json;
	json["uuid"] = pinholeUUID;
	json.write_to_string(m_pinhole_msg);

	return this;
}

std::string RoomUser::get_user_id() const
{
	return this->m_user_id;
}

std::string RoomUser::get_user_name() const
{
	return this->m_user_name;
}

std::string RoomUser::get_user_ip_addr() const
{
	return this->m_user_ip_addr;
}

std::string RoomUser::get_user_uuid() const
{
	return this->m_user_uuid;
}

uint32_t RoomUser::get_video_ssrc() const
{
	return this->m_video_ssrc;
}

uint32_t RoomUser::get_audio_ssrc() const
{
	return this->m_audio_ssrc;
}

std::string RoomUser::get_pinhole_uuid() const
{
	return this->m_pinhole_uuid;
}

RoomUser *RoomUser::create_user()
{
	RoomUser *usr = new (std::nothrow) RoomUser();
	if (!usr)
	{
		LOG_ERROR("Out of memory for creating RoomUser");
		return NULL;
	}

	return usr;
}

void RoomUser::nat_pinhole()
{
	if (m_major_receiver)
	{
		m_major_receiver->nat_pinhole(m_pinhole_msg);
	}

	if (m_audio_receiver)
	{
		m_audio_receiver->nat_pinhole(m_pinhole_msg);
	}
}