#ifndef _H_RTP_H264_FRAME_ASSEMBLER_H_
#define _H_RTP_H264_FRAME_ASSEMBLER_H_

#include <stdint.h>
#include "rtp_packet.h"

//the assembler buffer size
const int ASSEMBLER_BUFFER_SIZE = 1024 * 256;

/**
* h264 frame assembler from rtp packets
*/
class RTPH264FrameAssembler
{
public:
	RTPH264FrameAssembler();
	virtual ~RTPH264FrameAssembler();

	/**
	* @brief initalize the assembler
	*
	* @return true -- initialize successfully
	*         false -- initialize failed.
	*/
	bool initialize();

	/**
	* @brief push a rtp packet to the assembler
	* @param packet - the rtp packet
	*
	* @return true - if the packet was pushed, and no error occurs, return true
	* @return false - failed
	*/
	bool push_packet(RTPPacket* packet);

	/**
	* @brief get the h264 frame data pointer
	*
	* @return the frame data pointer
	*/
	uint8_t* get_frame_data();

	/**
	* @brief get the h264 frame data length
	*
	* @return the frame data length
	*/
	size_t get_frame_length();

private:

	//get the STAP-A data
	bool get_stapa_frame(uint8_t* data, size_t len);

	//get the FU-A data
	bool get_fua_frame(uint8_t* data, size_t len);

private:
	bool m_initialize;
	uint8_t* m_buffer;
	size_t m_buffer_used_len;
};

#endif