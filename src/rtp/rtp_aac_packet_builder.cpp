#include "rtp_aac_packet_builder.h"

#include <algorithm>
#include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "common_logger.h"
#include "codec_utils.h"

//the rtp packets buffer size
const int RTP_PACKETS_BUFFER_SIZE = 1024 * 128;
const int RTP_PACKET_SIZE = 1400;

RTPAACPacketBuilder::RTPAACPacketBuilder()
{
	m_ssrc = 0;
	m_load_type = 0;
	m_sequence = 0;
	m_timestamp = 0;
	m_ntp_timestamp = 0;
	m_marker = false;

	m_rtp_buffer = NULL;
	m_rtp_buffer_end = NULL;
	m_current_pos = NULL;

	m_initialize = false;
}

RTPAACPacketBuilder::~RTPAACPacketBuilder()
{
	if (m_rtp_buffer)
	{
		delete[] m_rtp_buffer;
		m_rtp_buffer = NULL;
	}
}

bool RTPAACPacketBuilder::init(uint16_t sequenceStart, uint32_t timestampStart)
{
	if (m_initialize)
	{
		return true;
	}

	m_rtp_buffer = new (std::nothrow)uint8_t[RTP_PACKETS_BUFFER_SIZE];
	if (!m_rtp_buffer)
	{
		LOG_ERROR("RTPPacketBuilder::Init(), out of memory");
		return false;
	}
	m_rtp_buffer_end = m_rtp_buffer + RTP_PACKETS_BUFFER_SIZE;

	this->m_sequence = sequenceStart;
	this->m_timestamp = timestampStart;

	m_initialize = true;
	return true;
}

void RTPAACPacketBuilder::set_ssrc(uint32_t newssrc)
{
	this->m_ssrc = newssrc;
}

uint32_t RTPAACPacketBuilder::get_ssrc() const
{
	return this->m_ssrc;
}

void RTPAACPacketBuilder::increment_timestamp_by(uint32_t inc)
{
	this->m_timestamp += inc;
}

bool RTPAACPacketBuilder::send_data(const uint8_t *data, size_t len)
{
    if(len + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) >= RTP_PACKETS_BUFFER_SIZE)
    {
        return false;
    }

	m_current_pos = m_rtp_buffer;

    RTPHeader* rtpHeader = (RTPHeader*)m_current_pos;
	rtpHeader->version = 2;
	rtpHeader->padding = 0;
	rtpHeader->extension = 1;
	rtpHeader->csrcCount = 0;
	rtpHeader->payloadType = 97;
	rtpHeader->marker = 0;
	rtpHeader->sequence = htons((uint16_t)(m_sequence & 0x0000FFFF));
	rtpHeader->timestamp = htonl(this->m_timestamp);
	rtpHeader->ssrc = htonl(this->m_ssrc);

	RTPExtensionHeader* extHeader = (RTPExtensionHeader*)(m_current_pos + sizeof(RTPHeader));
	extHeader->id = 0;
	extHeader->length = htons(3);
	extHeader->reserved = 0;
	extHeader->seqHigh16 = htons((uint16_t)((m_sequence >> 16) & 0x0000FFFF));
	extHeader->msw = 0;
	extHeader->lsw = 0;

	m_current_pos += sizeof(RTPHeader) + sizeof(RTPExtensionHeader);
    memcpy(m_current_pos, data, len);
    m_current_pos += len;

    m_sequence++;

	return true;
}

std::pair<const uint8_t*, int> RTPAACPacketBuilder::receive_rtp_packet()
{
	std::pair<const uint8_t*, int> result = std::make_pair(m_rtp_buffer, m_current_pos - m_rtp_buffer);

    return result;
}