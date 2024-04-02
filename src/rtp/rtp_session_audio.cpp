#include "rtp_session_audio.h"

#include <string.h>
#include <time.h>
#include "common_logger.h"

RTPSessionAudio::RTPSessionAudio()
{
	m_initialize = false;
	m_aac_rtp_builder = NULL;
	m_transmitter = NULL;
}

RTPSessionAudio::~RTPSessionAudio()
{
	if (m_aac_rtp_builder)
	{
		delete m_aac_rtp_builder;
	}

	if (m_transmitter)
	{
		delete m_transmitter;
	}
}

bool RTPSessionAudio::init(uint16_t sequenceStart, uint32_t timestampStart, uint32_t ssrc)
{
	if (m_initialize)
	{
		return true;
	}

	bool ret;

	m_aac_rtp_builder = new RTPAACPacketBuilder();
	if (!m_aac_rtp_builder)
	{
		LOG_ERROR("Create RTPAACPacketBuilder failed.");
		goto exitFlag;
	}
	ret = m_aac_rtp_builder->init(sequenceStart, timestampStart);
	if (!ret)
	{
		goto exitFlag;
	}
	m_aac_rtp_builder->set_ssrc(ssrc);

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

	if (m_aac_rtp_builder)
	{
		delete m_aac_rtp_builder;
		m_aac_rtp_builder = NULL;
	}

	if (m_transmitter)
	{
		delete m_transmitter;
		m_transmitter = NULL;
	}

	m_initialize = false;
	return false;
}

bool RTPSessionAudio::add_destination(const char *ip, const uint16_t &port)
{
	if (!m_initialize)
	{
		return false;
	}

	return m_transmitter->add_destination(ip, port);
}

bool RTPSessionAudio::delete_destination(const char *ip, const uint16_t &port)
{
	if (!m_initialize)
	{
		return false;
	}

	return m_transmitter->delete_destination(ip, port);
}

bool RTPSessionAudio::clear_destination()
{
	return m_transmitter->clear_destination();
}

bool RTPSessionAudio::send_aac_data(const uint8_t *data, size_t length)
{
	if (!m_initialize)
	{
		return false;
	}

	clock_t now = clock();
	uint32_t now_ms = (uint32_t)(now * 1000 / CLOCKS_PER_SEC);

	if (m_aac_rtp_builder->send_data(data, length))
	{
		std::pair<const uint8_t *, int> rtp = m_aac_rtp_builder->receive_rtp_packet();
		RTPHeader *rtpHeader = (RTPHeader *)rtp.first;
		rtpHeader->timestamp = htonl(now_ms);

		bool ret = m_transmitter->send_data(rtp.first, rtp.second);
		return ret;
	}

	return false;
}