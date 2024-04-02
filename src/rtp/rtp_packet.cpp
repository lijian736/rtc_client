#include "rtp_packet.h"

#include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "common_logger.h"

RTPPacket::RTPPacket()
{
    reset();
}

RTPPacket::~RTPPacket()
{
}

bool RTPPacket::parse(uint8_t *buffer, size_t bufferLen)
{
    if (bufferLen <= sizeof(RTPHeader))
    {
        LOG_ERROR("buffer size is less than RTPHeader length");
        return false;
    }

    this->m_packet = buffer;
	this->m_packet_len = bufferLen;

    RTPHeader *rtpHeader = (RTPHeader *)buffer;
    if (rtpHeader->version != 2)
    {
        LOG_ERROR("RTP Header version error");
        return false;
    }

    this->m_padding = (rtpHeader->padding != 0);
    this->m_extension = (rtpHeader->extension != 0);
    this->m_csrc_count = rtpHeader->csrcCount;
    this->m_marker = (rtpHeader->marker != 0);
    this->m_payload_type = rtpHeader->payloadType;
    this->m_sequence = ntohs(rtpHeader->sequence);
    this->m_timestamp = ntohl(rtpHeader->timestamp);
    this->m_ssrc = ntohl(rtpHeader->ssrc);

    uint32_t paddingBytes = 0;
    if (rtpHeader->padding)
    {
		paddingBytes = buffer[bufferLen - 1];
        if (paddingBytes == 0)
        {
            LOG_ERROR("padding length error");
            return false;
        }
    }

    RTPExtensionHeader *rtpExtHeader = NULL;
    if (this->m_extension)
    {
        rtpExtHeader = (RTPExtensionHeader *)(buffer + sizeof(RTPHeader) + (this->m_csrc_count * 4));
        this->m_extension_id = ntohs(rtpExtHeader->id);
        this->m_extension_len = ntohs(rtpExtHeader->length);

		if (this->m_extension_len != 3)
		{
			LOG_ERROR("extension length error");
			return false;
		}

        uint32_t high32 = ntohs(rtpExtHeader->seqHigh16);
		this->m_sequence = (high32 << 16) | this->m_sequence;
		this->m_reserved = ntohs(rtpExtHeader->reserved);
		this->m_msw = ntohl(rtpExtHeader->msw);
		this->m_lsw = ntohl(rtpExtHeader->lsw);
    }
    else
    {
		LOG_ERROR("no extension error");
		return false;
    }

    this->m_payload_len = bufferLen - paddingBytes - sizeof(RTPHeader) - (this->m_csrc_count * 4) - sizeof(RTPExtensionHeader);
    if (this->m_payload_len < 0)
    {
        LOG_ERROR("payload length error");
        return false;
    }

    this->m_payload = buffer + sizeof(RTPHeader) + (this->m_csrc_count * 4) + sizeof(RTPExtensionHeader);

    return true;
}

bool RTPPacket::has_padding() const
{
    return this->m_padding;
}

bool RTPPacket::has_extension() const
{
    return this->m_extension;
}

uint8_t RTPPacket::get_csrc_count() const
{
    return this->m_csrc_count;
}

bool RTPPacket::has_marker() const
{
    return this->m_marker;
}

uint8_t RTPPacket::get_payload_type() const
{
    return this->m_payload_type;
}

uint32_t RTPPacket::get_sequence() const
{
    return this->m_sequence;
}

uint32_t RTPPacket::get_timestamp() const
{
    return this->m_timestamp;
}

uint32_t RTPPacket::get_ssrc() const
{
    return this->m_ssrc;
}

void RTPPacket::set_ssrc(uint32_t newSsrc)
{
    RTPHeader *rtpHeader = (RTPHeader *)this->m_packet;
    rtpHeader->ssrc = htonl(newSsrc);
}

uint32_t RTPPacket::get_csrc(int index) const
{
    if (index >= this->m_csrc_count)
    {
        return 0;
    }

    uint32_t *csrcval = (uint32_t *)(this->m_packet + sizeof(RTPHeader) + index * 4);
    return ntohl(*csrcval);
}

uint8_t *RTPPacket::get_payload() const
{
    return this->m_payload;
}

size_t RTPPacket::get_payload_length() const
{
    return this->m_payload_len;
}

uint8_t * RTPPacket::get_packet() const
{
	return this->m_packet;
}
size_t RTPPacket::get_packet_length() const
{
	return this->m_packet_len;
}

uint16_t RTPPacket::get_extension_id() const
{
    return this->m_extension_id;
}

uint16_t RTPPacket::get_extension_length() const
{
    return this->m_extension_len;
}

uint16_t RTPPacket::get_reserved() const
{
	return this->m_reserved;
}

uint32_t RTPPacket::get_msw() const
{
	return this->m_msw;
}

uint32_t RTPPacket::get_lsw() const
{
	return this->m_lsw;
}

void RTPPacket::reset()
{
    m_padding = false;
    m_csrc_count = 0;
    m_marker = false;
    m_extension = false;
    m_payload_type = 0;
    m_sequence = 0;
    m_timestamp = 0;
    m_ssrc = 0;
    m_payload_len = 0;
    m_payload = NULL;
    m_packet = NULL;
	m_packet_len = 0;
    m_extension_id = 0;
    m_extension_len = 0;
	m_reserved = 0;
	m_msw = 0;
	m_lsw = 0;
}