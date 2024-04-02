#ifndef _H_RTP_AAC_PACKET_BUILDER_H_
#define _H_RTP_AAC_PACKET_BUILDER_H_

#include <vector>
#include <stdint.h>
#include "rtp_packet.h"

/**
* the rtp packet builder for AAC frame data
*/
class RTPAACPacketBuilder
{
public:
	RTPAACPacketBuilder();
	virtual ~RTPAACPacketBuilder();

	/**
	 * @brief initialize the builder
	 * @param sequenceStart -- the start of sequence
	 * @param timestampStart -- the start of timestamp
	 * @return true -- initialize successful
	 *         false -- initialize fail
	 */
	bool init(uint16_t sequenceStart, uint32_t timestampStart);

	/**
	 * @brief sent data to the builder.
	 *
	 * @param data - the data pointer
	 *        len - the data length
	 *
	 */
	bool send_data(const uint8_t *data, size_t len);

	/**
	 * @brief Receive the rtp packet
	 * 
	 * @return the rtp data and size
	 */
	std::pair<const uint8_t*, int> receive_rtp_packet();

	/**
	 * @brief set the ssrc
	 */
	void set_ssrc(uint32_t ssrc);

	/**
	 * @brief get the random ssrcc
	 * @return the ssrc value
	 */
	uint32_t get_ssrc() const;

	/**
	 * @brief this function increments the timestamp with inc
	 * @param inc -- the increment value
	 */
	void increment_timestamp_by(uint32_t inc);

private:
	bool m_initialize;
	uint32_t m_ssrc;
	uint8_t m_load_type;
	uint32_t m_sequence;
	uint32_t m_timestamp;
	uint64_t m_ntp_timestamp;
	bool m_marker;

	//the rtp packets buffer
	uint8_t* m_rtp_buffer;
	//the rtp packets buffer end
	const uint8_t* m_rtp_buffer_end;
	//the rtp packets buffer current position pointer
	uint8_t* m_current_pos;
};

#endif