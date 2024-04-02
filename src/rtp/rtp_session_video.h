#ifndef _H_RTP_SESSION_VIDEO_H_
#define _H_RTP_SESSION_VIDEO_H_

#include <list>
#include <stdint.h>
#include "rtp_h264_packet_builder.h"
#include "rtp_transmitter_v4.h"

class RTPSessionVideo
{
public:
	RTPSessionVideo();
	virtual ~RTPSessionVideo();

	bool is_initialize() const
	{
		return this->m_initialize;
	}

	/**
	 * @brief initialize the rtp session
	 * @param sequenceStart -- the start of sequence
	 * @param timestampStart -- the start of timestamp
	 * @param ssrc -- the rtp ssrc
	 *
	 * @return if initialize successfully, return true otherwise return false
	 */
	bool init(uint16_t sequenceStart, uint32_t timestampStart, uint32_t ssrc);

	/**
	 * @brief add the rtp destination address
	 *
	 * @param ip -- the destination ip address
	 *        port -- the destination port
	 *
	 * @return true - successful
	 * @return false - fail
	 */
	bool add_destination(const char *ip, const uint16_t &port);

	/**
	 * @brief delete the rtp destination address
	 *
	 * @param ip -- the destination ip address
	 *        port -- the destination port
	 *
	 * @return true - successful
	 * @return false - fail
	 */
	bool delete_destination(const char *ip, const uint16_t &port);

	/**
	 * @brief clear the destinations
	 * 
	 * @return true 
	 * @return false 
	 */
	bool clear_destination();

	/**
	 * @brief send data to destination address.
	 * NOTE: the data was not copied to the session. the session only
	 *  holds a reference to the data. Make sure that the data exists until
	 *  the send_data() function returns.
	 *
	 * @param data -- the data should send.the data is h264 data.
	 * @param length -- the h264 data length
	 *
	 * @return true - successful
	 * @return false - fail
	 */
	bool send_h264_data(const uint8_t *data, size_t length);

private:
	//whether the session was initialized
	bool m_initialize;

	//the socket transmitter
	RTPTransmitterV4 *m_transmitter;

	//the h264 rtp packet builder
	RTPH264PacketBuilder *m_h264_rtp_builder;

	//the rtp packets vector, it was used to build the rtp packet for sending to remote peer
	// pair: first -- rtp packet data; second -- rtp packet data length
	std::vector<std::pair<const uint8_t *, int>> m_rtp_send_packets;
};

#endif