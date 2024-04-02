#include "codec_utils.h"

#include <iostream>
#include <math.h>
#include <string.h>


/**
* @brief find start code
* @param start -- the start position
* @param end -- the end position
*
* @return the position of start code, if not found return @param end
*
*/
static const uint8_t *AVCFindStartCodeInternal(const uint8_t *start, const uint8_t *end)
{
	const uint8_t *a = start + 4 - ((intptr_t)start & 3);

	for (end -= 3; start < a && start < end; start++)
	{
		if (start[0] == 0 && start[1] == 0 && start[2] == 1)
		{
			return start;
		}
	}

	for (end -= 3; start < end; start += 4)
	{
		uint32_t x = *(const uint32_t *)start;

		if ((x - 0x01010101) & (~x) & 0x80808080)
		{
			if (start[1] == 0)
			{
				if (start[0] == 0 && start[2] == 1)
				{
					return start;
				}
				if (start[2] == 0 && start[3] == 1)
				{
					return start + 1;
				}
			}

			if (start[3] == 0)
			{
				if (start[2] == 0 && start[4] == 1)
				{
					return start + 2;
				}
				if (start[4] == 0 && start[5] == 1)
				{
					return start + 3;
				}
			}
		}
	}

	for (end += 3; start < end; start++)
	{
		if (start[0] == 0 && start[1] == 0 && start[2] == 1)
		{
			return start;
		}
	}

	return end + 3;
}

const uint8_t* avc_find_start_code(const uint8_t *start, const uint8_t *end)
{
	const uint8_t *pos = AVCFindStartCodeInternal(start, end);
	if (start < pos && pos < end && !pos[-1])
	{
		pos--;
	}

	return pos;
}

bool avc_find_key_frame(const uint8_t *data, size_t size)
{
	const uint8_t *nalStart;
	const uint8_t *nalEnd;
	const uint8_t *end = data + size;
	int type;

	nalStart = avc_find_start_code(data, end);
	while (true)
	{
		while (nalStart < end && !*(nalStart++))
			;

		if (nalStart == end)
		{
			break;
		}

		type = nalStart[0] & 0x1F;

		if (type == 5 || type == 1)
		{
			return (type == 5);
		}

		nalEnd = avc_find_start_code(nalStart, end);
		nalStart = nalEnd;
	}

	return false;
}

bool avc_find_sps_pps_idr(const uint8_t* data, size_t size)
{
	const uint8_t *nalStart;
	const uint8_t *nalEnd;
	const uint8_t *end = data + size;
	int type;

	nalStart = avc_find_start_code(data, end);
	while (true)
	{
		while (nalStart < end && !*(nalStart++))
			;

		if (nalStart == end)
		{
			break;
		}

		type = nalStart[0] & 0x1F;

		if (type == 5 || type == 7 || type == 8)
		{
			return true;
		}

		nalEnd = avc_find_start_code(nalStart, end);
		nalStart = nalEnd;
	}

	return false;
}

int count_avc_key_frames(const uint8_t *data, size_t size)
{
	int count = 0;
	const uint8_t *nalStart;
	const uint8_t *nalEnd;
	const uint8_t *end = data + size;
	int type;

	nalStart = avc_find_start_code(data, end);
	while (true)
	{
		while (nalStart < end && !*(nalStart++))
			;

		if (nalStart == end)
		{
			break;
		}

		type = nalStart[0] & 0x1F;

		if (type == 5)
		{
			count++;
		}

		nalEnd = avc_find_start_code(nalStart, end);
		nalStart = nalEnd;
	}

	return count;
}

int count_frames(const uint8_t *data, size_t size)
{
	int count = 0;
	const uint8_t *nalStart;
	const uint8_t *nalEnd;
	const uint8_t *end = data + size;

	nalStart = avc_find_start_code(data, end);
	while (true)
	{
		while (nalStart < end && !*(nalStart++))
			;

		if (nalStart == end)
		{
			break;
		}

		count++;
		nalEnd = avc_find_start_code(nalStart, end);
		nalStart = nalEnd;
	}

	return count;
}

bool parse_adts_header(const uint8_t* data, size_t data_len, struct ADTSHeader* header)
{
	if (data_len < 7)
	{
		return false;
	}

	memset(header, 0, sizeof(struct ADTSHeader));
	if ((data[0] != 0xFF) || ((data[1] & 0xF0) != 0xF0))
	{
		return false;
	}

	header->id = (data[1] & 0x08) >> 3;
	header->layer = (data[1] & 0x06) >> 1;
	header->protection_absent = data[1] & 0x01;
	header->profile = (data[2] & 0xC0) >> 6;
	header->sampling_freq_index = (data[2] & 0x3C) >> 2;
	header->private_bit = (data[2] & 0x02) >> 1;
	header->channel_cfg = ((data[2] & 0x01) << 2) | ((data[3] & 0xC0) >> 6);
	header->original_copy = (data[3] & 0x20) >> 5;
	header->home = (data[3] & 0x10) >> 4;
	header->copyright_identification_bit = (data[3] & 0x08) >> 3;
	header->copyright_identification_start = (data[3] & 0x04) >> 2;
	header->aac_frame_length = ((((uint16_t)data[3]) & 0x03) << 11) |
		((((uint16_t)data[4]) & 0xFF) << 3) |
		((data[5] & 0xE0) >> 5);
	header->adts_buffer_fullness = ((((uint16_t)data[5]) & 0x1F) << 6) |
		((((uint16_t)data[6]) & 0xFC) >> 2);
	header->number_of_raw_data_blocks_in_frame = (data[6] & 0x03);

	return true;
}

int count_aac_frames(const uint8_t* data, size_t size)
{
	bool parseRet = false;
	int count = 0;
	struct ADTSHeader header;
	size_t currentPos = 0;

	while (currentPos + 7 <= size)
	{
		parseRet = parse_adts_header(data + currentPos, 7, &header);
		if (parseRet)
		{
			count++;
			currentPos += (header.aac_frame_length);
		}
		else
		{
			break;
		}
	}

	return count;
}


static uint32_t Ue(const uint8_t *pBuff, size_t nLen, uint32_t &nStartBit)
{
	uint32_t nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;

	uint64_t dwRet = 0;
	for (uint32_t i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}

	return (1 << nZeroNum) - 1 + dwRet;
}

static int32_t Se(uint8_t *pBuff, size_t nLen, uint32_t &nStartBit)
{
	int32_t UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int32_t nValue = (int32_t)ceil(k / 2);
	if (UeVal % 2 == 0)
	{
		nValue = -nValue;
	}
	return nValue;
}

uint64_t u(uint32_t BitCount, uint8_t * buf, uint32_t &nStartBit)
{
	uint64_t dwRet = 0;
	for (uint32_t i = 0; i < BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

void de_emulation_prevention(uint8_t* buf, size_t* buf_size)
{
	int32_t i = 0, j = 0;
	uint8_t* tmp_ptr = NULL;
	size_t tmp_buf_size = 0;
	int32_t val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i < (tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j < tmp_buf_size - 1; j++)
			{
				tmp_ptr[j] = tmp_ptr[j + 1];
			}

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}

bool h264_decode_sps(uint8_t *data, size_t size, int* width, int *height, int*fps)
{
	uint32_t StartBit = 0;
	de_emulation_prevention(data, &size);

	int forbidden_zero_bit = u(1, data, StartBit);
	int nal_ref_idc = u(2, data, StartBit);
	int nal_unit_type = u(5, data, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, data, StartBit);
		int constraint_set0_flag = u(1, data, StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, data, StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, data, StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, data, StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, data, StartBit);
		int level_idc = u(8, data, StartBit);

		int seq_parameter_set_id = Ue(data, size, StartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			int chroma_format_idc = Ue(data, size, StartBit);
			if (chroma_format_idc == 3)
				int residual_colour_transform_flag = u(1, data, StartBit);
			int bit_depth_luma_minus8 = Ue(data, size, StartBit);
			int bit_depth_chroma_minus8 = Ue(data, size, StartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, data, StartBit);
			int seq_scaling_matrix_present_flag = u(1, data, StartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++)
				{
					seq_scaling_list_present_flag[i] = u(1, data, StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4 = Ue(data, size, StartBit);
		int pic_order_cnt_type = Ue(data, size, StartBit);
		if (pic_order_cnt_type == 0)
		{
			int log2_max_pic_order_cnt_lsb_minus4 = Ue(data, size, StartBit);
		}
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, data, StartBit);
			int offset_for_non_ref_pic = Se(data, size, StartBit);
			int offset_for_top_to_bottom_field = Se(data, size, StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(data, size, StartBit);

			int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
			{
				offset_for_ref_frame[i] = Se(data, size, StartBit);
			}
			delete[] offset_for_ref_frame;
		}
		int num_ref_frames = Ue(data, size, StartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, data, StartBit);
		int pic_width_in_mbs_minus1 = Ue(data, size, StartBit);
		int pic_height_in_map_units_minus1 = Ue(data, size, StartBit);

		*width = (pic_width_in_mbs_minus1 + 1) * 16;
		*height = (pic_height_in_map_units_minus1 + 1) * 16;

		int frame_mbs_only_flag = u(1, data, StartBit);
		if (!frame_mbs_only_flag)
		{
			int mb_adaptive_frame_field_flag = u(1, data, StartBit);
		}

		int direct_8x8_inference_flag = u(1, data, StartBit);
		int frame_cropping_flag = u(1, data, StartBit);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset = Ue(data, size, StartBit);
			int frame_crop_right_offset = Ue(data, size, StartBit);
			int frame_crop_top_offset = Ue(data, size, StartBit);
			int frame_crop_bottom_offset = Ue(data, size, StartBit);
		}
		int vui_parameter_present_flag = u(1, data, StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, data, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, data, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, data, StartBit);
					int sar_height = u(16, data, StartBit);
				}
			}
			int overscan_info_present_flag = u(1, data, StartBit);
			if (overscan_info_present_flag)
			{
				int overscan_appropriate_flagu = u(1, data, StartBit);
			}
			int video_signal_type_present_flag = u(1, data, StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, data, StartBit);
				int video_full_range_flag = u(1, data, StartBit);
				int colour_description_present_flag = u(1, data, StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, data, StartBit);
					int transfer_characteristics = u(8, data, StartBit);
					int matrix_coefficients = u(8, data, StartBit);
				}
			}
			int chroma_loc_info_present_flag = u(1, data, StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(data, size, StartBit);
				int chroma_sample_loc_type_bottom_field = Ue(data, size, StartBit);
			}
			int timing_info_present_flag = u(1, data, StartBit);
			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, data, StartBit);
				int time_scale = u(32, data, StartBit);
				*fps = time_scale / (2 * num_units_in_tick);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

const uint8_t* aac_find_frame(const uint8_t *data, size_t size)
{
	for (size_t i = 0; i < size - 1; i++)
	{
		if ((data[i] == 0xFF) && ((data[i + 1] & 0xF0) == 0xF0))
		{
			return (data + i);
		}
	}

	return NULL;
}

void dump_adts_header(struct ADTSHeader* header)
{
	std::cout << "adts id: " << (uint16_t)header->id << std::endl;
	std::cout << "adts layer: " << (uint16_t)header->layer << std::endl;
	std::cout << "adts protection_absent: " << (uint16_t)header->protection_absent << std::endl;
	std::cout << "adts profile: " << (uint16_t)header->profile << std::endl;
	std::cout << "adts sampling frequence index: " << (uint16_t)header->sampling_freq_index << std::endl;
	std::cout << "adts private bit: " << (uint16_t)header->private_bit << std::endl;
	std::cout << "adts channel config: " << (uint16_t)header->channel_cfg << std::endl;
	std::cout << "adts original copy: " << (uint16_t)header->original_copy << std::endl;
	std::cout << "adts home: " << (uint16_t)header->home << std::endl;
	std::cout << "adts copyright identification bit: " << (uint16_t)header->copyright_identification_bit << std::endl;
	std::cout << "adts copyright identification start: " << (uint16_t)header->copyright_identification_start << std::endl;
	std::cout << "adts aac frame length: " << header->aac_frame_length << std::endl;
	std::cout << "adts buffer fullness: " << header->adts_buffer_fullness << std::endl;
	std::cout << "adts number of raw data blocks in frame: " << (uint16_t)header->number_of_raw_data_blocks_in_frame << std::endl;
}

#ifdef _WIN32

std::tuple<bool, uint8_t*, size_t, uint8_t> avc_read_nalu(const uint8_t *data, size_t size)
{
	const uint8_t *nalStart;
	const uint8_t *nalEnd;
	const uint8_t *end = data + size;

	nalStart = avc_find_start_code(data, end);
	while (nalStart < end && !*(nalStart++))
		;
	if (nalStart == end)
	{
		return std::make_tuple(false, (uint8_t*)NULL, 0, 0);
	}

	uint8_t type = nalStart[0] & 0x1F;
	nalEnd = avc_find_start_code(nalStart, end);
	if (nalEnd == end)
	{
		return std::make_tuple(false, (uint8_t*)NULL, 0, 0);
	}
	else
	{
		return std::make_tuple(true, (uint8_t*)nalStart, (size_t)(nalEnd - nalStart), type);
	}
}

#endif