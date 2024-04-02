#ifndef _H_RTP_SESSION_AUDIO_H_
#define _H_RTP_SESSION_AUDIO_H_

#include <list>
#include <stdint.h>
#include "rtp_aac_packet_builder.h"
#include "rtp_transmitter_v4.h"

class RTPSessionAudio
{
public:
	RTPSessionAudio();
	virtual ~RTPSessionAudio();

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
	 * @param data -- the AAC data
	 * @param length -- the AAC length
	 * @return true - successful
	 * @return false - failed
	 */
	bool send_aac_data(const uint8_t *data, size_t length);

private:
	//whether the session was initialized
	bool m_initialize;

	//the socket transmitter
	RTPTransmitterV4 *m_transmitter;

	//the AAC rtp packet builder
	RTPAACPacketBuilder *m_aac_rtp_builder;
};

#endif