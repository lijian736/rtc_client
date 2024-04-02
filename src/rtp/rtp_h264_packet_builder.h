#ifndef _H_RTP_H264_PACKET_BUILDER_H_
#define _H_RTP_H264_PACKET_BUILDER_H_

#include <vector>
#include <stdint.h>
#include "rtp_packet.h"

//the rtp packet size, it's less than mtu
const int RTP_PACKET_SIZE = 1400;

//the rtp packet payload size
const int RTP_PAYLOAD_SIZE = RTP_PACKET_SIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader);

//the rtp packets buffer size
const int RTP_PACKETS_BUFFER_SIZE = 1024 * 256;

//the nalu buffer size
const int NALU_BUFFER_SIZE = 1024 * 256;

/**
* the rtp packet builder for H264 frame data
*/
class RTPH264PacketBuilder
{
public:
	RTPH264PacketBuilder();
	virtual ~RTPH264PacketBuilder();

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
	 * the data is not copied to the builder.
	 * the builder only has pointer to the data.
	 *
	 * @param data - the data pointer
	 *        len - the data length
	 *
	 */
	bool send_data(const uint8_t *data, size_t len);

	/**
	 * @brief Receive the rtp packet
	 * 
	 * @param packets -- the rtp packet data vectors
	 *
	 */
	bool receive_rtp_packets(std::vector<std::pair<const uint8_t*, int> >& packets);

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

	/**
	 * @brief build the rtp FU-A packets
	 *
	 * @param data -- the data
	 *        len -- the data length
	 *        rtpvec-- the rtp data vector, output parameter
	 */
	bool build_fua_packet(const uint8_t* data, int len, std::vector<std::pair<const uint8_t*, int> >& rtpvec);

	/**
	* @brief build the rtp packets
	*
	* @param vec -- the <data,length> vectors
	*        rtpvec -- the rtp data vector, output parameter
	*/
	bool build_packet(std::vector<std::pair<const uint8_t*, int> >& datavec,
		std::vector<std::pair<const uint8_t*, int> >& rtpvec);

private:
	bool m_initialize;
	uint32_t m_ssrc;
	uint8_t m_load_type;
	uint32_t m_sequence;
	uint32_t m_timestamp;
	uint64_t m_ntp_timestamp;
	bool m_marker;

	//the NALUs vector. the NALU does not include the start code 00 00 00 01
	//the pair consists of the nalu pointer and the nalu length
	std::vector<std::pair<const uint8_t*, int> > m_nalu_vec;

	//the rtp packets buffer
	uint8_t* m_rtp_buffer;
	//the rtp packets buffer end
	const uint8_t* m_rtp_buffer_end;
	//the rtp packets buffer current position pointer
	uint8_t* m_current_pos;
};

#endif
