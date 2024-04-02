#ifndef _H_RTP_PACKET_H_
#define _H_RTP_PACKET_H_

#include <stdint.h>
#include <stddef.h>

/* rtp header
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           timestamp                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |           synchronization source (SSRC) identifier            |
 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 |            contributing source (CSRC) identifiers             |
 |                             ....                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/* rtp header extension
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |       defined by profile      |              length           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |       reserved                |high 16 bits of sequence number|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |          NTP timestamp, most significant word                 |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |          NTP timestamp, least significant word                |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * RTP HEADER
 */
struct RTPHeader
{
#ifdef SYS_BIG_ENDIAN
	uint8_t version : 2;
	uint8_t padding : 1;
	uint8_t extension : 1;
	uint8_t csrcCount : 4;

	uint8_t marker : 1;
	uint8_t payloadType : 7;
#else
	uint8_t csrcCount : 4;
	uint8_t extension : 1;
	uint8_t padding : 1;
	uint8_t version : 2;

	uint8_t payloadType : 7;
	uint8_t marker : 1;
#endif

	uint16_t sequence;
	uint32_t timestamp;
	uint32_t ssrc;
};

/**
 * RTP extension header
 */
struct RTPExtensionHeader
{
	uint16_t id;
	uint16_t length;
	uint16_t reserved;
	uint16_t seqHigh16;
	uint32_t msw;
	uint32_t lsw;
};

/**
 * @brief the RTP packet
 * 
 */
class RTPPacket
{
public:
	RTPPacket();
	~RTPPacket();

	bool parse(uint8_t *buffer, size_t bufferLen);

	bool has_padding() const;
	bool has_extension() const;
	uint8_t get_csrc_count() const;
	bool has_marker() const;
	uint8_t get_payload_type() const;
	uint32_t get_sequence() const;
	uint32_t get_timestamp() const;
	uint32_t get_ssrc() const;
	void set_ssrc(uint32_t newSsrc);
	uint32_t get_csrc(int index) const;

	uint8_t *get_payload() const;
	size_t get_payload_length() const;

	uint16_t get_extension_id() const;
	uint16_t get_extension_length() const;
	uint16_t get_reserved() const;
	uint32_t get_msw() const;
	uint32_t get_lsw() const;

	uint8_t *get_packet() const;
	size_t get_packet_length() const;

	void reset();

private:
	bool m_padding;
	bool m_extension;
	uint8_t m_csrc_count;
	bool m_marker;
	uint8_t m_payload_type;
	uint32_t m_sequence;
	uint32_t m_timestamp;
	uint32_t m_ssrc;
	uint16_t m_extension_id;
	uint16_t m_extension_len;
	uint16_t m_reserved;
	uint32_t m_msw;
	uint32_t m_lsw;

	uint8_t * m_payload;
	size_t m_payload_len;

	uint8_t * m_packet;
	size_t m_packet_len;
};

#endif