#ifndef _H_RTP_SESSION_RECEIVER_H_
#define _H_RTP_SESSION_RECEIVER_H_

#include <list>
#include <stdint.h>
#include "rtp_transmitter_v4.h"
#include "rtp_packet.h"

//the rtp receive buffer size. it was used to receive data from remote peer
const int RTP_RECV_BUFFER_SIZE = 1024 * 2;

class RTPSessionReceiver
{
public:
	RTPSessionReceiver();
	virtual ~RTPSessionReceiver();

	bool is_initialize() const
	{
		return this->m_initialize;
	}

	/**
	 * @brief initialize the rtp session
	 *
	 * @param reorder -- whether the rtp receive session re-order the rtp packets
	 * @param recvParams -- the recv transmission parameters
	 * @param pinholeIP -- the NAT pinhole ip address
	 * @param pinholePort -- the NAt pinhole port
	 *
	 * @return if initialize successfully, return true otherwise return false
	 */
	bool init(bool reorder, RTPTransParamsV4 *recvParams, const char *pinholeIP, const uint16_t &pinholePort);

	/**
	 * @brief the receive NAT pinhole
	 * @param message -- the pinhole message
	 */
	void nat_pinhole(const std::string &message);

	/**
	 * @brief receive rtp packet.
	 * @param timeout_us -- the timeout in microseconds
	 * @return the rtp packet pointer. if receive failed, returns NULL pointer.
	 */
	RTPPacket *receive_rtp_packet(int64_t timeout_us);

	/**
	* @brief receive rtp packet from the re-order packets buffer.
	*
	* @param reorderLen -- the re-order buffer len. if the packets number in the 
	* packets buffer is less than reorderLen, then the function returns NULL.
	* @param timeout_us -- the timeout in microseconds
	*
	* @return the rtp packet pointer. if receive failed, returns NULL pointer.
	*/
	RTPPacket *receive_rtp_packet(int reorderLen, int64_t timeout_us);

	/**
	* @brief end of receiving rtp packet.
	* Note: This function MUST be called when you call receive_rtp_packet();
	*
	* @param packet -- the rtp packete which the function ReceiveRTPPacket() returns.
	*/
	void end_receive_rtp_packet(RTPPacket *packet);

private:
	//whether the session was initialized
	bool m_initialize;

	//the socket transmitter
	RTPTransmitterV4 *m_transmitter;

	//whether reorder the rtp packets by their sequences.
	bool m_reorder;
	//the reorder packets list
	std::list<RTPPacket *> m_reorder_list;

	//the rtp packet, it was used to receive data from remote peer
	RTPPacket *m_rtp_packet;
	//the rtp receive buffer, it was used to receive data from remote peer
	uint8_t *m_recv_buffer;
};

#endif