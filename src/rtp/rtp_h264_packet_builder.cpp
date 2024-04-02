#include "rtp_h264_packet_builder.h"

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

RTPH264PacketBuilder::RTPH264PacketBuilder()
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

RTPH264PacketBuilder::~RTPH264PacketBuilder()
{
	if (m_rtp_buffer)
	{
		delete[] m_rtp_buffer;
		m_rtp_buffer = NULL;
	}
}

bool RTPH264PacketBuilder::init(uint16_t sequenceStart, uint32_t timestampStart)
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

void RTPH264PacketBuilder::set_ssrc(uint32_t newssrc)
{
	this->m_ssrc = newssrc;
}

uint32_t RTPH264PacketBuilder::get_ssrc() const
{
	return this->m_ssrc;
}

void RTPH264PacketBuilder::increment_timestamp_by(uint32_t inc)
{
	this->m_timestamp += inc;
}

bool RTPH264PacketBuilder::build_fua_packet(const uint8_t* data, int len,
	std::vector<std::pair<const uint8_t*, int> >& rtpvec)
{
	const uint8_t fnri = data[0] & 0xE0;
	const uint8_t type = data[0] & 0x1F;
	bool isStart = true;
	data++;
	len--;

	int leftLen = len;

	while (true)
	{
		if (m_current_pos + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) >= m_rtp_buffer_end)
		{
			return false;
		}

		const uint8_t* const start = m_current_pos;

		RTPHeader* rtpHeader = (RTPHeader*)m_current_pos;
		rtpHeader->version = 2;
		rtpHeader->padding = 0;
		rtpHeader->extension = 1;
		rtpHeader->csrcCount = 0;
		rtpHeader->payloadType = 96;
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

		if (m_current_pos + 2 >= m_rtp_buffer_end)
		{
			return false;
		}

		int copyLen = ((RTP_PAYLOAD_SIZE - 2) < leftLen ? (RTP_PAYLOAD_SIZE - 2) : leftLen);
		//FU indicator
		*(m_current_pos++) = fnri | 0x1c;
		if (isStart)
		{
			//FU header -- start
			*(m_current_pos++) = 0x80 | type;
			isStart = false;
		}
		else
		{
			if (copyLen == leftLen)
			{
				//FU header  -- end
				*(m_current_pos++) = 0x40 | type;
			}
			else
			{
				//FU header -- middle
				*(m_current_pos++) = 0x00 | type;
			}
		}

		if (m_current_pos + copyLen >= m_rtp_buffer_end)
		{
			return false;
		}
		else
		{
			memcpy(m_current_pos, data + (len - leftLen), copyLen);
			m_current_pos += copyLen;
			rtpvec.push_back(std::make_pair(start, m_current_pos - start));
			m_sequence++;
			leftLen -= copyLen;
			if (leftLen <= 0)
			{
				break;
			}
		}
	}

	return true;
}

bool RTPH264PacketBuilder::build_packet(std::vector<std::pair<const uint8_t*, int> >& datavec,
	std::vector<std::pair<const uint8_t*, int> >& rtpvec)
{
	if (m_current_pos + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) >= m_rtp_buffer_end)
	{
		return false;
	}

	const uint8_t* const start = m_current_pos;

	RTPHeader* rtpHeader = (RTPHeader*)m_current_pos;
	rtpHeader->version = 2;
	rtpHeader->padding = 0;
	rtpHeader->extension = 1;
	rtpHeader->csrcCount = 0;
	rtpHeader->payloadType = 96;
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

	//single nal unit packet
	if (datavec.size() == 1)
	{
		const uint8_t* data = datavec[0].first;
		int dataLen = datavec[0].second;
		if (m_current_pos + dataLen < m_rtp_buffer_end)
		{
			memcpy(m_current_pos, data, dataLen);
			m_current_pos += dataLen;
			rtpvec.push_back(std::make_pair(start, m_current_pos - start));
			m_sequence++;

			return true;
		}
		else
		{
			return false;
		}
	}
	else  //STAP-A
	{
		std::vector<std::pair<const uint8_t*, int> >::iterator it;
		for (it = datavec.begin(); it != datavec.end(); it++)
		{
			const uint8_t* data = it->first;
			int dataLen = it->second;
			if (m_current_pos + dataLen < m_rtp_buffer_end)
			{
				if (it == datavec.begin())
				{
					//STAP-A
					*m_current_pos = 0x18;
					m_current_pos++;
				}
				//size
				*(m_current_pos++) = (dataLen >> 8) & 0xFF;
				*(m_current_pos++) = dataLen & 0xFF;

				memcpy(m_current_pos, data, dataLen);
				m_current_pos += dataLen;
			}
			else
			{
				return false;
			}
		}

		rtpvec.push_back(std::make_pair(start, m_current_pos - start));
		m_sequence++;

		return true;
	}
}

bool RTPH264PacketBuilder::send_data(const uint8_t *data, size_t len)
{
	this->m_nalu_vec.clear();

	const uint8_t* end = data + len;
	const uint8_t* startCode;

	startCode = avc_find_start_code(data, end);
	while (startCode < end)
	{
		while (!*(startCode++));
		const uint8_t* nextStart = avc_find_start_code(startCode, end);
		int naluLen = (int)(nextStart - startCode);
		if (naluLen > 0)
		{
			//if (startCode[0] == 0x41 || startCode[0] == 0x61 || startCode[0] == 0x67 || startCode[0] == 0x68 || startCode[0] == 0x65)
			{
				this->m_nalu_vec.push_back(std::make_pair(startCode, naluLen));
			}
		}

		startCode = nextStart;
	}

	return true;
}

bool RTPH264PacketBuilder::receive_rtp_packets(std::vector<std::pair<const uint8_t*, int> >& packets)
{
	m_current_pos = m_rtp_buffer;

	int totalLen = 0;
	std::vector<std::pair<const uint8_t*, int> > tmp;

	std::vector<std::pair<const uint8_t*, int> >::iterator it;
	for (it = m_nalu_vec.begin(); it != m_nalu_vec.end(); it++)
	{
		const uint8_t* naluData = it->first;
		int naluLen = it->second;

		if (naluLen > RTP_PAYLOAD_SIZE)
		{
			//single nalu or STAP-A
			if (tmp.size() > 0)
			{
				if (!build_packet(tmp, packets))
				{
					return false;
				}
				tmp.clear();
				totalLen = 0;
			}

			//FU-A
			if (!build_fua_packet(naluData, naluLen, packets))
			{
				return false;
			}
		}
		else
		{
			if (totalLen < RTP_PAYLOAD_SIZE && RTP_PAYLOAD_SIZE < totalLen + naluLen)
			{
				//single nalu or STAP-A
				if (tmp.size() > 0)
				{
					if (!build_packet(tmp, packets))
					{
						return false;
					}
					tmp.clear();
				}

				totalLen = naluLen;
				tmp.push_back(std::make_pair(naluData, naluLen));
			}
			else
			{
				totalLen += naluLen;
				tmp.push_back(std::make_pair(naluData, naluLen));
			}
		}
	}

	//single nalu or STAP-A
	if (tmp.size() > 0)
	{
		if (!build_packet(tmp, packets))
		{
			return false;
		}
		tmp.clear();
	}

	return true;
}