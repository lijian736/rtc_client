#ifndef _H_CODEC_UTILS_H_
#define _H_CODEC_UTILS_H_

#include <stdint.h>

#ifdef _WIN32
#include <tuple>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h> 
}
#include "sample_utils.h"
#endif

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/** 
 RFC-6184 -- RTP Payload Format for H.264 Video


 H264 NALU(network abstract layer unit)
 IDR: instantaneous decoding refresh is I frame, but I frame not must be IDR

 |0|1|2|3|4|5|6|7|
 |F|NRI|Type     |

 F: forbidden_zero_bit, must be 0
 NRI:nal_ref_idc, the importance indicator
 Type: nal_unit_type

 0x67(103) (0 11 00111) SPS     very important       type = 7
 0x68(104) (0 11 01000) PPS     very important       type = 8
 0x65(101) (0 11 00101) IDR     very important       type = 5
 0x61(97)  (0 11 00001) I       important            type = 1 it's not IDR
 0x41(65)  (0 10 00001) P       important            type = 1
 0x01(1)   (0 00 00001) B       not important        type = 1
 0x06(6)   (0 00 00110) SEI     not important        type = 6

 NAL Unit Type     Packet Type      Packet Type Name
 -------------------------------------------------------------
 0                 reserved         -
 1-23              NAL unit         Single NAL unit packet
 24                STAP-A           Single-time aggregation packet
 25                STAP-B           Single-time aggregation packet
 26                MTAP16           Multi-time aggregation packet
 27                MTAP24           Multi-time aggregation packet
 28                FU-A             Fragmentation unit
 29                FU-B             Fragmentation unit
 30-31             reserved
*/

/**
* @brief find avc start code position
* @param start -- the start position
* @param end -- the end position, NOT included
* @return  the position of start code, if the start code non found, then return @param end
*/
const uint8_t* avc_find_start_code(const uint8_t *start, const uint8_t *end);
bool avc_find_key_frame(const uint8_t *data, size_t size);
int count_avc_key_frames(const uint8_t *data, size_t size);
int count_frames(const uint8_t *data, size_t size);
bool avc_find_sps_pps_idr(const uint8_t* data, size_t size);

/**
* @brief count the aac frames
*
* @param data -- the aac data pointer
* @param size -- the aac data length
* @return int -- the aac frames count
*/
int count_aac_frames(const uint8_t* data, size_t size);

/**
*@brief decode the H.264 sps
*
*@param data -- the data pointer
*@param size -- the data length
*@param width -- [output] the width of the video
*@param height -- [output] the height of the video
*@param fps -- [output] the fps of the video
*
*@return true -- decode successfully, false -- decode failed
*/
bool h264_decode_sps(uint8_t *data, size_t size, int* width, int *height, int *fps);

/**
The ADTS Profile:
Object Type ID             Audio Object Type               memo
1                          AAC Main
2                          AAC LC                          most commonly used profile
3                          AAC LTR
4                          SBR
5                          AAC calable
*/

/**
The ADTS Sampling Frequency Index
Sampling frequency index               value
0x00, 0000                             96000
0x1, 0001                              88000
0x2, 0010                              64000
0x3, 0011                              48000
0x4, 0100                              44100      //cd
0x5, 0101                              32000
0x6, 0110                              24000
0x7, 0111                              22000
0x8, 1000                              16000
ox9, 1001                              12000
0xa, 1010                              11025
0xb, 1011                              8000       //telephone
0xc, 1100                              7350
0xd, 1101                              reserved
0xe, 1110                              reserved
0xf, 1111                              escape value
*/

/**
Channel Configuration
value          number of channels         audio syntactic list in order received    tips
0              -                          -
1              1                          single channel element                    1
2              2                          channel pair element                      2
3              3                          single channel element                    1+2
channel pair element
4              4                          single channel element                    1+2+1
channel pair element
single channel element
5              5                          single channel element                    1+2+2
channel pair element
channel pair element
6              5+1                        single channel element                    1+2+2+1
channel pair element
channel pair element
lfe channel element
7              7+1                        single channel element                    1+2+2+2+1
channel pair element
channel pair element
channel pair element
lfe channel element
8-15           -                          -                                         reserved
*/
//The AAC(Advanced Audio Coding) ADTS(Audio Data Transport Stream) Header
struct ADTSHeader
{
	//the fixed header follows
	uint16_t syncword;               //12 bit, synchronized word, all bits must be 1, '1111 1111 1111'£¬it denotes the start of ADTS header
	uint8_t id;                      //1 bit, the MPEG Version, 0 for MPEG-4£¬1 for MPEG-2
	uint8_t layer;                   //2 bit, always'00'
	uint8_t protection_absent;       //1 bit, set to 1 if no CRC£¬set to 0 if there is CRC
	uint8_t profile;                 //2 bit, the MPEG-4 Audio Object Type minus 1, 01 is AAC LC
	uint8_t sampling_freq_index;     //4 bit, the frequency index, 15 is forbidden
	uint8_t private_bit;             //1 bit, set to 0 when encoding, ignore when decoding
	uint8_t channel_cfg;             //3 bit, the channels.
	uint8_t original_copy;           //1 bit, set to 0 when encoding, ignore when decoding
	uint8_t home;                    //1 bit, set to 0 when encoding, ignore when decoding

									 //the variable header follows
	uint8_t copyright_identification_bit;      //1 bit, set to 0 when encoding, ignore when decoding
	uint8_t copyright_identification_start;    //1 bit, set to 0 when encoding, ignore when decoding
	uint16_t aac_frame_length;                 //13 bit, the AAC length, this value must include 7 or 9
											   // bytes of header length: aac_frame_length = (protection_absent == 1 ? 7 : 9) + size(AACFrame)
	uint16_t adts_buffer_fullness;             //11 bit, set to 0x7FF for variable bitrate, set to 0x000 for fixed bitrate

											   /* number_of_raw_data_blocks_in_frame
											   * the frame count is: number_of_raw_data_blocks_in_frame + 1
											   * there is 1 frame if number_of_raw_data_blocks_in_frame == 0
											   * An AAC frame contains 1024 samples
											   */
	uint8_t number_of_raw_data_blocks_in_frame;      //2 bit
													 //uint16_t crc;        //crc if protection_absent is 0
};

/**
* @brief parse the adts header
*
* @param data -- the data pointer
* @param data_len -- the data length
* @param header -- the adts header pointer
* @return true -- parse successful
* @return false -- parse failed
*/
bool parse_adts_header(const uint8_t* data, size_t data_len, struct ADTSHeader* header);

const uint8_t* aac_find_frame(const uint8_t *data, size_t size);

/**
* @brief dump the adts header
*
* @param header -- the adts header pointer
*/
void dump_adts_header(struct ADTSHeader* header);

//H264 nalu data
struct H264Nalu
{
	uint8_t *buffer;
	size_t used_length;
	int64_t duration; //in 100 nanoseconds
	uint64_t start_time;
	uint64_t stop_time;

	H264Nalu()
	{
		buffer = new uint8_t[1024 * 64];
		used_length = 0;
		duration = 0;
		start_time = stop_time = 0;
	}

	bool append(uint8_t* data, int len, uint64_t start_time_p, uint64_t stop_time_p)
	{
		if (used_length + len < 1024 * 64)
		{
			memcpy(buffer + used_length, data, len);
			used_length += len;
			start_time = start_time_p;
			stop_time = stop_time_p;

			return true;
		}
		else
		{
			return false;
		}
	}

	void reset()
	{
		used_length = 0;
		duration = 0;
		start_time = stop_time = 0;
	}

	~H264Nalu()
	{
		if (buffer)
		{
			delete[] buffer;
			buffer = NULL;
		}
	}
};

//AAC ADTS data
struct ADTS
{
	uint8_t *buffer;
	size_t used_length;
	int duration; //in sample rate

	ADTS()
	{
		buffer = new uint8_t[1024 * 64];
		used_length = 0;
		duration = 0;
	}

	bool append(uint8_t* data, int dataLen, int duration_p)
	{
		if (used_length + dataLen < 1024 * 64)
		{
			memcpy(buffer + used_length, data, dataLen);
			used_length += dataLen;
			duration += duration_p;

			return true;
		}
		else
		{
			return false;
		}
	}

	void reset()
	{
		used_length = 0;
		duration = 0;
	}

	~ADTS()
	{
		if (buffer)
		{
			delete[] buffer;
			buffer = NULL;
		}
	}
};

#ifdef _WIN32

/**
* @brief read a nalu
* @param data -- the data pointer
* @param size -- the data size
* @return std::tuple
*  0: bool, true-read successfully, false-nalu not found
*  1: uint8_t*, the nalu data pointer
*  2: the data length
*  3: the nalu type
*/
std::tuple<bool, uint8_t*, size_t, uint8_t> avc_read_nalu(const uint8_t *data, size_t size);

struct VideoFrameRender
{
	uint8_t *data[3];
	int linesize[3];
	uint32_t width;
	uint32_t height;
	uint64_t timestamp;
	uint64_t startTime;
	uint64_t stopTime;
	VideoFormat format;
	int ffmpegFormat;
	bool isFFmpeg;

	VideoFrameRender()
	{
		data[0] = data[1] = data[2] = 0;
		linesize[0] = linesize[1] = linesize[2] = 0;
		width = height = 0;
		timestamp = 0;
		startTime = stopTime = 0;
		format = VideoFormat::Unknown;
		isFFmpeg = false;
	}

	inline bool InitFromFFmpeg(AVFrame* frame)
	{
		isFFmpeg = true;

		for (int i = 0; i < 3; i++)
		{
			data[i] = frame->data[i];
			linesize[i] = frame->linesize[i];
		}

		return true;
	}

	inline bool InitFrom(uint8_t* data_p, uint32_t len, VideoFormat fmt, uint32_t width_p, uint32_t height_p)
	{
		data[0] = data[1] = data[2] = 0;
		linesize[0] = linesize[1] = linesize[2] = 0;

		isFFmpeg = false;
		if (fmt == VideoFormat::RGB24)
		{
			data[0] = data_p;
			linesize[0] = width_p * 3;
		}
		else if (fmt == VideoFormat::ARGB32 || fmt == VideoFormat::XRGB32)
		{
			data[0] = data_p;
			linesize[0] = width_p * 4;
		}
		else if (fmt == VideoFormat::RGB555 || fmt == VideoFormat::RGB565)
		{
			data[0] = data_p;
			linesize[0] = width_p * 2;
		}
		else if (fmt == VideoFormat::IYUV)
		{
			data[0] = data_p;
			linesize[0] = width_p;

			data[1] = data_p + width_p * height_p;
			linesize[1] = width_p / 2;

			data[2] = data_p + width_p * height_p * 5 / 4;
			linesize[2] = width_p / 2;
		}
		else if (fmt == VideoFormat::NV12)
		{
			data[0] = data_p;
			linesize[0] = width_p;

			data[1] = data_p + width_p * height_p;
			linesize[1] = width_p ;
		}
		else if (fmt == VideoFormat::YV12)
		{
			data[0] = data_p;
			linesize[0] = width_p;

			data[2] = data_p + width_p * height_p;
			linesize[2] = width_p / 2;

			data[1] = data_p + width_p * height_p * 5 / 4;
			linesize[1] = width_p / 2;
		}
		else if (fmt == VideoFormat::AYUV)
		{
			data[0] = data_p;
			linesize[0] = width_p * 3;
		}
		else if (fmt == VideoFormat::YVYU || fmt == VideoFormat::YUY2 || fmt == VideoFormat::UYVY)
		{
			data[0] = data_p;
			linesize[0] = width_p * 2;
		}
		else
		{
			return false;
		}

		return true;
	}
};

#define ALIGN_SIZE(size, align) (((size) + (align - 1)) & (~(align - 1)))

struct YUV420PEncodeFrame
{
	uint8_t *buffer;
	uint8_t *data[3];
	int linesize[3];
	uint32_t width;
	uint32_t height;
	uint64_t timestamp;

	YUV420PEncodeFrame()
	{
		int size = ALIGN_SIZE(1024 * 1024 * 6, 32);
		buffer = (uint8_t*)_aligned_malloc(size, 32);

		data[0] = data[1] = data[2] = 0;
		linesize[0] = linesize[1] = linesize[2] = 0;
		width = height = 0;
		timestamp = 0;
	}

	~YUV420PEncodeFrame()
	{
		if (buffer)
		{
			_aligned_free(buffer);
		}
	}

	void InitFrom(struct VideoFrameRender& vfr)
	{
		if (!buffer)
		{
			return;
		}

		data[0] = data[1] = data[2] = 0;
		linesize[0] = linesize[1] = linesize[2] = 0;

		width = vfr.width;
		height = vfr.height;
		timestamp = vfr.timestamp;

		uint32_t offset[3] = { 0 };

		int size = width * height;
		size = ALIGN_SIZE(size, 32);
		offset[0] = size;

		size += width * height / 4;
		size = ALIGN_SIZE(size, 32);
		offset[1] = size;

		size += width * height / 4;
		size = ALIGN_SIZE(size, 32);

		data[0] = buffer;
		linesize[0] = vfr.linesize[0];

		data[1] = data[0] + offset[0];
		linesize[1] = vfr.linesize[1];

		data[2] = data[0] + offset[1];
		linesize[2] = vfr.linesize[2];

		memcpy(data[0], vfr.data[0], width * height);
		memcpy(data[1], vfr.data[1], width * height / 4);
		memcpy(data[2], vfr.data[2], width * height / 4);
	}
};
#endif

#endif
