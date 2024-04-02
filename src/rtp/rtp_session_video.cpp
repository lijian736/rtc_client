#include "rtp_session_video.h"

#include <string.h>
#include <time.h>
#include "common_logger.h"

RTPSessionVideo::RTPSessionVideo()
{
	m_initialize = false;
	m_h264_rtp_builder = NULL;
	m_transmitter = NULL;
}

RTPSessionVideo::~RTPSessionVideo()
{
	if (m_h264_rtp_builder)
	{
		delete m_h264_rtp_builder;
	}

	if (m_transmitter)
	{
		delete m_transmitter;
	}
}

bool RTPSessionVideo::init(uint16_t sequenceStart, uint32_t timestampStart, uint32_t ssrc)
{
	if (m_initialize)
	{
		return true;
	}

	bool ret;

	m_h264_rtp_builder = new RTPH264PacketBuilder();
	if (!m_h264_rtp_builder)
	{
		LOG_ERROR("Create RTPH264PacketBuilder failed.");
		goto exitFlag;
	}
	ret = m_h264_rtp_builder->init(sequenceStart, timestampStart);
	if (!ret)
	{
		goto exitFlag;
	}
	m_h264_rtp_builder->set_ssrc(ssrc);

	m_transmitter = new RTPTransmitterV4();
	if (!m_transmitter)
	{
		LOG_ERROR("Create RTPTransmitterV4 failed.");
		goto exitFlag;
	}

	ret = m_transmitter->init(NULL);
	if (!ret)
	{
		goto exitFlag;
	}

	m_initialize = true;
	return true;

exitFlag:
	if (m_h264_rtp_builder)
	{
		delete m_h264_rtp_builder;
		m_h264_rtp_builder = NULL;
	}

	if (m_transmitter)
	{
		delete m_transmitter;
		m_transmitter = NULL;
	}

	m_initialize = false;
	return false;
}

bool RTPSessionVideo::add_destination(const char *ip, const uint16_t &port)
{
	if (!m_initialize)
	{
		return false;
	}

	return m_transmitter->add_destination(ip, port);
}

bool RTPSessionVideo::delete_destination(const char *ip, const uint16_t &port)
{
	if (!m_initialize)
	{
		return false;
	}

	return m_transmitter->delete_destination(ip, port);
}

bool RTPSessionVideo::clear_destination()
{
	return m_transmitter->clear_destination();
}

bool RTPSessionVideo::send_h264_data(const uint8_t *data, size_t length)
{
	if (!m_initialize)
	{
		return false;
	}

	clock_t now = clock();
	uint32_t now_ms = (uint32_t)(now * 1000 / CLOCKS_PER_SEC);

	bool ret = m_h264_rtp_builder->send_data(data, length);
	if (!ret)
	{
		return false;
	}

	m_rtp_send_packets.clear();
	ret = m_h264_rtp_builder->receive_rtp_packets(m_rtp_send_packets);
	if (!ret)
	{
		LOG_ERROR("generate rtp packet error.");
		return false;
	}

	uint8_t num = (uint8_t)m_rtp_send_packets.size();
	std::vector<std::pair<const uint8_t *, int>>::iterator it;
	for (it = m_rtp_send_packets.begin(); it != m_rtp_send_packets.end(); it++)
	{
		RTPExtensionHeader *header = (RTPExtensionHeader *)(it->first + sizeof(RTPHeader));
		header->reserved = htons(num);
		header->seqHigh16 = 0;
		RTPHeader *rtpHeader = (RTPHeader *)it->first;
		if (it == m_rtp_send_packets.end() - 1)
		{
			rtpHeader->marker = 1;
		}

		rtpHeader->timestamp = htonl(now_ms);

		ret = m_transmitter->send_data(it->first, it->second);
		if (!ret)
		{
			return false;
		}
	}

	return true;
}