#include "rtp_session_receiver.h"

#include <string.h>
#include <time.h>
#include "common_logger.h"

RTPSessionReceiver::RTPSessionReceiver()
{
	m_initialize = false;
	m_transmitter = NULL;
	m_reorder = false;
	m_rtp_packet = NULL;
	m_recv_buffer = NULL;
}

RTPSessionReceiver::~RTPSessionReceiver()
{
	if (m_transmitter)
	{
		delete m_transmitter;
	}

	if (m_rtp_packet)
	{
		delete m_rtp_packet;
	}

	if (m_recv_buffer)
	{
		delete[] m_recv_buffer;
	}

	std::list<RTPPacket *>::iterator it;
	for (it = m_reorder_list.begin(); it != m_reorder_list.end(); it++)
	{
		uint8_t *ptr = (*it)->get_packet();
		delete[] ptr;

		delete (*it);
	}
}

bool RTPSessionReceiver::init(bool reorder, RTPTransParamsV4 *recvParams, const char* pinholeIP, const uint16_t& pinholePort)
{
	if (m_initialize)
	{
		return true;
	}

	bool ret;

	m_reorder = reorder;
	m_transmitter = new RTPTransmitterV4();
	if (!m_transmitter)
	{
		LOG_ERROR("Create RTPTransmitterV4 failed.");
		goto exitFlag;
	}

	ret = m_transmitter->init(recvParams);
	if (!ret)
	{
		goto exitFlag;
	}

	ret = m_transmitter->add_destination(pinholeIP, pinholePort);
	if (!ret)
	{
		goto exitFlag;
	}

	m_rtp_packet = new RTPPacket();
	if (!m_rtp_packet)
	{
		goto exitFlag;
	}

	m_recv_buffer = new uint8_t[RTP_RECV_BUFFER_SIZE];
	if (!m_recv_buffer)
	{
		goto exitFlag;
	}

	m_initialize = true;
	return true;

exitFlag:

	if (m_transmitter)
	{
		delete m_transmitter;
		m_transmitter = NULL;
	}

	if (m_rtp_packet)
	{
		delete m_rtp_packet;
		m_rtp_packet = NULL;
	}

	if (m_recv_buffer)
	{
		delete[] m_recv_buffer;
		m_recv_buffer = NULL;
	}

	m_initialize = false;
	return false;
}

void RTPSessionReceiver::nat_pinhole(const std::string& message)
{
	m_transmitter->send_data((const uint8_t*)message.c_str(), message.size());
}

RTPPacket *RTPSessionReceiver::receive_rtp_packet(int64_t timeout_us)
{
	if (!m_initialize)
	{
		return NULL;
	}

	int receivedLen = m_transmitter->receive_data(m_recv_buffer, RTP_RECV_BUFFER_SIZE, timeout_us);
	if (receivedLen == 0)
	{
		return NULL;
	}

	bool ret = m_rtp_packet->parse(m_recv_buffer, receivedLen);
	if (!ret)
	{
		return NULL;
	}

	return m_rtp_packet;
}

RTPPacket *RTPSessionReceiver::receive_rtp_packet(int reorderLen, int64_t timeout_us)
{
	if (reorderLen <= 0 || !m_reorder)
	{
		return receive_rtp_packet(timeout_us);
	}

	int receivedLen = m_transmitter->receive_data(m_recv_buffer, RTP_RECV_BUFFER_SIZE, timeout_us);
	if (m_reorder_list.size() == 0)
	{
		uint8_t *ptr = new uint8_t[receivedLen];
		if (ptr)
		{
			RTPPacket *tmpPacket = new RTPPacket();
			if (tmpPacket)
			{
				m_reorder_list.push_back(tmpPacket);
			}
			else
			{
				delete[] ptr;
			}
		}
	}else if (receivedLen > 0)
	{
		uint8_t *ptr = new uint8_t[receivedLen];
		if (ptr)
		{
			RTPPacket *tmpPacket = new RTPPacket();
			if (tmpPacket)
			{
				memcpy(ptr, m_recv_buffer, receivedLen);
				bool ret = tmpPacket->parse(ptr, receivedLen);
				if (!ret)
				{
					delete[] ptr;
					delete tmpPacket;
				}
				else
				{
					//insert to list
					uint32_t seq = tmpPacket->get_sequence();
					std::list<RTPPacket *>::iterator it;
					std::list<RTPPacket *>::iterator start;
					bool done = false;

					it = m_reorder_list.end();
					--it;
					start = m_reorder_list.begin();

					while (!done)
					{
						RTPPacket *p;
						uint32_t seqnr;

						p = *it;
						seqnr = p->get_sequence();
						if (seqnr > seq)
						{
							if (it != start)
							{
								--it;
							}
							else
							{
								done = true;
								m_reorder_list.push_front(tmpPacket);
							}
						}
						else if (seqnr < seq)
						{
							++it;
							m_reorder_list.insert(it, tmpPacket);
							done = true;
						}
						else
						{
							done = true;
							//the sequences are equal, drop the packet
							delete[] ptr;
							delete tmpPacket;
						}
					}
				}
			}
			else
			{
				delete[] ptr;
			}
		}
	}

	if ((int)m_reorder_list.size() > reorderLen)
	{
		RTPPacket *packet = m_reorder_list.front();
		m_reorder_list.pop_front();

		return packet;
	}

	return NULL;
}

void RTPSessionReceiver::end_receive_rtp_packet(RTPPacket *packet)
{
	if (packet && packet != m_rtp_packet)
	{
		uint8_t *ptr = packet->get_packet();
		delete[] ptr;

		delete packet;
	}
}