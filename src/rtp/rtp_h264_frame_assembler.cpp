#include "rtp_h264_frame_assembler.h"

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

namespace
{
	const uint8_t START_CODE[] = { 0, 0, 0, 1 };
	const uint8_t SHORT_START_CODE[] = { 0, 0, 1 };
}

RTPH264FrameAssembler::RTPH264FrameAssembler()
	:m_initialize(false), m_buffer(NULL), m_buffer_used_len(0)
{
}

RTPH264FrameAssembler::~RTPH264FrameAssembler()
{
	if (m_buffer)
	{
		delete[] m_buffer;
	}
}

bool RTPH264FrameAssembler::initialize()
{
	if (m_initialize)
	{
		return true;
	}

	m_buffer = new uint8_t[ASSEMBLER_BUFFER_SIZE];
	if (!m_buffer)
	{
		return false;
	}

	m_initialize = true;
	return true;
}

bool RTPH264FrameAssembler::push_packet(RTPPacket* packet)
{
	if (!m_initialize)
	{
		return false;
	}

	uint8_t* payload;
	size_t payloadLen;
	uint8_t nal;
	uint8_t type;

	payload = packet->get_payload();
	payloadLen = packet->get_payload_length();
	if (payloadLen == 0)
	{
		return false;
	}

	nal = payload[0];
	type = nal & 0x1F;

	if (type >= 1 && type <= 23)
	{
		type = 1;
	}

	switch (type)
	{
	case 0:
	case 1:  //single nalu
		if (nal == 0x65 || ((nal == 0x61 || nal == 0x41) && ((payload[1] & 0x80) == 0x0)))
		{
			if (payloadLen + sizeof(SHORT_START_CODE) > ASSEMBLER_BUFFER_SIZE)
			{
				m_buffer_used_len = 0;
				return false;
			}

			m_buffer_used_len = payloadLen + sizeof(SHORT_START_CODE);
			memcpy(m_buffer, SHORT_START_CODE, sizeof(SHORT_START_CODE));
			memcpy(m_buffer + sizeof(SHORT_START_CODE), payload, payloadLen);
		}
		else
		{
			if (payloadLen + sizeof(START_CODE) > ASSEMBLER_BUFFER_SIZE)
			{
				m_buffer_used_len = 0;
				return false;
			}

			m_buffer_used_len = payloadLen + sizeof(START_CODE);
			memcpy(m_buffer, START_CODE, sizeof(START_CODE));
			memcpy(m_buffer + sizeof(START_CODE), payload, payloadLen);
		}

		return true;

	case 24:  //STAP-A
		payload++;
		payloadLen--;

		return get_stapa_frame(payload, payloadLen);

	case 28:  //FU-A

		return get_fua_frame(payload, payloadLen);

	default:
		LOG_ERROR("Unknown h264 rtp packet format.");
		return false;
	}
}

bool RTPH264FrameAssembler::get_stapa_frame(uint8_t* data, size_t len)
{
	const uint8_t *src = data;
	int srcLen = (int)len;

	m_buffer_used_len = 0;

	while (srcLen > 2)
	{
		uint16_t* nalSizePtr = (uint16_t*)src;
		uint16_t nalSize = ntohs(*nalSizePtr);

		src += 2;
		srcLen -= 2;
		if (nalSize <= srcLen)
		{
			if (src[0] == 0x65 || ((src[0] == 0x61 || src[0] == 0x41) && ((src[1] & 0x80) == 0x0)))
			{
				memcpy(m_buffer + m_buffer_used_len, SHORT_START_CODE, sizeof(SHORT_START_CODE));
				m_buffer_used_len += sizeof(SHORT_START_CODE);

				memcpy(m_buffer + m_buffer_used_len, src, nalSize);
				m_buffer_used_len += nalSize;
			}
			else
			{
				memcpy(m_buffer + m_buffer_used_len, START_CODE, sizeof(START_CODE));
				m_buffer_used_len += sizeof(START_CODE);

				memcpy(m_buffer + m_buffer_used_len, src, nalSize);
				m_buffer_used_len += nalSize;
			}
		}
		else
		{
			//invalid format
			m_buffer_used_len = 0;
			return false;
		}

		src += nalSize;
		srcLen -= nalSize;
	}

	if (m_buffer_used_len > ASSEMBLER_BUFFER_SIZE)
	{
		m_buffer_used_len = 0;
		return false;
	}

	return true;
}

bool RTPH264FrameAssembler::get_fua_frame(uint8_t* data, size_t len)
{
	uint8_t fuIndicator;
	uint8_t fuHeader;
	uint8_t startbit;
	uint8_t endbit;
	uint8_t nalType;
	uint8_t nal;

	if (len < 3 || len > ASSEMBLER_BUFFER_SIZE)
	{
		m_buffer_used_len = 0;
		return false;
	}

	fuIndicator = data[0];
	fuHeader = data[1];
	startbit = fuHeader >> 7;
	endbit = fuHeader & 0x40;
	nalType = fuHeader & 0x1f;
	nal = (fuIndicator & 0xe0) | nalType;

	data += 2;
	len -= 2;

	if (startbit)
	{
		if (data[0] == 0x65 || ((data[0] == 0x61 || data[0] == 0x41) && ((data[1] & 0x80) == 0x0)))
		{
			m_buffer_used_len = sizeof(SHORT_START_CODE) + 1 + len;
			memcpy(m_buffer, SHORT_START_CODE, sizeof(SHORT_START_CODE));
			memcpy(m_buffer + sizeof(SHORT_START_CODE), &nal, 1);
			memcpy(m_buffer + sizeof(SHORT_START_CODE) + 1, data, len);
		}
		else
		{
			m_buffer_used_len = sizeof(START_CODE) + 1 + len;
			memcpy(m_buffer, START_CODE, sizeof(START_CODE));
			memcpy(m_buffer + sizeof(START_CODE), &nal, 1);
			memcpy(m_buffer + sizeof(START_CODE) + 1, data, len);
		}

		return false;
	}
	else if(endbit)
	{
		memcpy(m_buffer + m_buffer_used_len, data, len);
		m_buffer_used_len += len;

		return true;
	}
	else
	{
		memcpy(m_buffer + m_buffer_used_len, data, len);
		m_buffer_used_len += len;

		return false;
	}
}

uint8_t* RTPH264FrameAssembler::get_frame_data()
{
	return m_buffer;
}

size_t RTPH264FrameAssembler::get_frame_length()
{
	return m_buffer_used_len;
}
