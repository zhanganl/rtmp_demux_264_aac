//
//  sps_decode.c
//  GJCaptureTool
//
//  Created by mac on 17/1/17.
//  Copyright © 2017年 MinorUncle. All rights reserved.
//

#include "parse_h264.h"

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "libnav",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , "libnav",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , "libnav",__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , "libnav",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , "libnav",__VA_ARGS__)
#define EXPORT 
#else
#define LOGE  printf
#endif

//无符号指数Golomb熵编码
static UINT Ue(BYTE *pBuff, UINT nLen, UINT** nStartBit)
{
	//计算0bit的个数
	UINT nZeroNum = 0;
	UINT* tmp = *nStartBit;
	while ((*tmp) < nLen * 8)
	{
		if (pBuff[(*tmp) / 8] & (0x80 >> ((*tmp) % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		(*tmp)++;
	}
	(*tmp)++;//0000 0001
	//0000 000167

	//计算结果
	DWORD dwRet = 0;
	UINT i = 0;
	for (i = 0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[(*tmp) / 8] & (0x80 >> ((*tmp) % 8)))
		{
			dwRet += 1;
		}
		(*tmp)++;
	}
	return (1 << nZeroNum) - 1 + (UINT)dwRet;
}

//有符号指数Golomb熵编码
static int Se(BYTE *pBuff, UINT nLen, UINT** nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}


//读进连续的若干Bit，并把它们解释为无符号整数
static UINT u(UINT BitCount, BYTE * buf, UINT** nStartBit)
{
	UINT dwRet = 0;
	UINT* tmp = *nStartBit;
	UINT i = 0;
	for (i = 0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[(*tmp) / 8] & (0x80 >> ((*tmp) % 8)))
		{
			dwRet += 1;
		}
		(*tmp)++;
	}
	return dwRet;
}

/**
* H264的NAL起始码防竞争机制
*
* @param buf SPS数据内容
*
* @无返回值
*/
static void de_emulation_prevention(BYTE* buf, unsigned int* buf_size)
{
	unsigned int i = 0, j = 0;
	BYTE* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i<(tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j<tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}

/**
* 解码SPS,获取视频图像宽、高信息
*
* @param buf SPS数据内容
* @param nLen SPS数据的长度
* @param width 图像宽度
* @param height 图像高度

* @成功则返回1 , 失败则返回0
*/
int h264_decode_sps(BYTE * buf, unsigned int nLen, int* width, int* height, int* fps,int* stride)
{
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored"-Wunused-variable"
	int* Width = width;
	int* Height = height;
	int* Stride = stride;
	LOGE("zhanganl h264_decode_sps 111");
	UINT StartBit = 0;
	UINT* pStartBit = &StartBit;
	de_emulation_prevention(buf, &nLen);
	LOGE("zhanganl h264_decode_sps 22222");
	int forbidden_zero_bit = u(1, buf, &pStartBit);
	int nal_ref_idc = u(2, buf, &pStartBit);
	int nal_unit_type = u(5, buf, &pStartBit);
	if (nal_unit_type == 7)
	{
		LOGE("zhanganl h264_decode_sps 333333");
		int profile_idc = u(8, buf, &pStartBit);
		int constraint_set0_flag = u(1, buf, &pStartBit);	//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, buf, &pStartBit);	//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, buf, &pStartBit);	//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, buf, &pStartBit);	//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, buf, &pStartBit);
		int level_idc = u(8, buf, &pStartBit);
		LOGE("zhanganl h264_decode_sps 444444");
		int seq_parameter_set_id = Ue(buf, nLen, &pStartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			int chroma_format_idc = Ue(buf, nLen, &pStartBit);
			if (chroma_format_idc == 3)
			{
				int residual_colour_transform_flag = u(1, buf, &pStartBit);
			}
			int bit_depth_luma_minus8 = Ue(buf, nLen, &pStartBit);
			int bit_depth_chroma_minus8 = Ue(buf, nLen, &pStartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, buf, &pStartBit);
			int seq_scaling_matrix_present_flag = u(1, buf, &pStartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				int i = 0;
				for (i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, &pStartBit);
					LOGE("seq_scaling_list_present_flag[%d]: %d", i, seq_scaling_list_present_flag[i]);
				}
			}
		}
		LOGE("zhanganl h264_decode_sps 555555555");
		int log2_max_frame_num_minus4 = Ue(buf, nLen, &pStartBit);
		int pic_order_cnt_type = Ue(buf, nLen, &pStartBit);
		if (pic_order_cnt_type == 0){
			int  log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, &pStartBit);
		}
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, buf, &pStartBit);
			int offset_for_non_ref_pic = Se(buf, nLen, &pStartBit);
			int offset_for_top_to_bottom_field = Se(buf, nLen, &pStartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, &pStartBit);

			//int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
			int i = 0;
			int tmp=0;
			for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				/*offset_for_ref_frame[i]*/ tmp = Se(buf, nLen, &pStartBit);
			//delete [] offset_for_ref_frame;
			LOGE("tmp:%d", tmp);
		}
		LOGE("zhanganl h264_decode_sps 666666");
		int num_ref_frames = Ue(buf, nLen, &pStartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, buf, &pStartBit);
		int pic_width_in_mbs_minus1 = Ue(buf, nLen, &pStartBit);
		int pic_height_in_map_units_minus1 = Ue(buf, nLen, &pStartBit);
		LOGE("zhanganl h264_decode_sps 77777777777");
		(*Width) = (pic_width_in_mbs_minus1 + 1) * 16;
		(*Height) = (pic_height_in_map_units_minus1 + 1) * 16;
		LOGE("zhanganl h264_decode_sps aaaaaaaaaa");
		int frame_mbs_only_flag = u(1, buf, &pStartBit);
		LOGE("zhanganl h264_decode_sps bbbbbbbbb");
		if (!frame_mbs_only_flag){
			int mb_adaptiv_frame_field_flag = u(1, buf, &pStartBit);
			LOGE("zhanganl h264_decode_sps ccccccccccc");
		}
		LOGE("zhanganl h264_decode_sps 888888");
		int direct_8x8_inference_flag = u(1, buf, &pStartBit);
		int frame_cropping_flag = u(1, buf, &pStartBit);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset = Ue(buf, nLen, &pStartBit);
			int frame_crop_right_offset = Ue(buf, nLen, &pStartBit);
			int frame_crop_top_offset = Ue(buf, nLen, &pStartBit);
			int frame_crop_bottom_offset = Ue(buf, nLen, &pStartBit);
			(*Stride) = (*Width) + 2 * frame_crop_right_offset + 2 * frame_crop_left_offset;
			LOGE("zhanganl h264_decode_sps frame_cropping_flag:%d left_offset:%d  right_offset:%d  top_offset:%d  bottom_offset:%d", frame_cropping_flag, frame_crop_left_offset, frame_crop_right_offset, frame_crop_top_offset, frame_crop_bottom_offset);
		}
		else
		{
			*Stride = *Width;
		}
		
		int vui_parameter_present_flag = u(1, buf, &pStartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, buf, &pStartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, buf, &pStartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, buf, &pStartBit);
					int sar_height = u(16, buf, &pStartBit);
				}
			}
			int overscan_info_present_flag = u(1, buf, &pStartBit);
			if (overscan_info_present_flag)
				u(1, buf, &pStartBit);
			int video_signal_type_present_flag = u(1, buf, &pStartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, buf, &pStartBit);
				int video_full_range_flag = u(1, buf, &pStartBit);
				int colour_description_present_flag = u(1, buf, &pStartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, buf, &pStartBit);
					int transfer_characteristics = u(8, buf, &pStartBit);
					int matrix_coefficients = u(8, buf, &pStartBit);
				}
			}
			LOGE("zhanganl h264_decode_sps 9999999999");
			int chroma_loc_info_present_flag = u(1, buf, &pStartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(buf, nLen, &pStartBit);
				int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, &pStartBit);
			}
			int timing_info_present_flag = u(1, buf, &pStartBit);
			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, buf, &pStartBit);
				int time_scale = u(32, buf, &pStartBit);
				if (fps) {
					*fps = time_scale / (2 * num_units_in_tick);
				}
			}
		}
		LOGE("zhanganl h264_decode_sps 101010");
		return 1;
	}
	else
		return 0;
//#pragma clang diagnostic pop 
}


void find_pp_sps_pps(int *isKey, uint8_t* data, int size, uint8_t **pp, uint8_t **sps, int *spsSize, uint8_t** pps, int *ppsSize, uint8_t** sei, int *seiSize){
	uint8_t* p = data;

	uint8_t* preNAL = p;
	int* preSize = NULL;
	int headSize = 4;
	if (isKey) {
		*isKey = 0;
	}
	while (p<data + size) {
		if (p[0] == 0 && p[1] == 0) {
			if (p[2] == 0 && p[3] == 1) {
				headSize = 4;
			}
			else if (p[2] == 1){
				headSize = 3;
			}
			else{
				p += 2;
				continue;
			}
			if (preSize) {
				*preSize = (int)(p - preNAL);
			}
			switch (p[headSize] & 0x1f) {
			case 7:
			{
				preNAL = p;
				preSize = spsSize;
				if (sps) {
					*sps = p;
				}
				break;
			}
			case 8:
			{
				preNAL = p;
				preSize = ppsSize;
				if (pps) {
					*pps = p;
				}
				break;
			}
			case 6:
			{
				preNAL = p;
				preSize = seiSize;
				if (sei) {
					*sei = p;
				}
				break;
			}
			case 5:
				if (isKey) {
					*isKey = 1;
				}
			case 1:
			{
				if (pp) {  //当没有idr时，退出，避免多余的查找,
					//                        preNAL = p + headSize;
					*pp = p;
				}
				return;
			}

			default:
				break;
			}
			p += headSize;
			continue;
		}
		p++;
	}
}

static const int mpeg4audio_sample_rates[16] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000, 7350
};
//int size, rdb, ch, sr;
//int aot, crc_abs;
//
//if (get_bits(gbc, 12) != 0xfff)
//return AAC_AC3_PARSE_ERROR_SYNC;
//
//skip_bits1(gbc);             /* id */
//skip_bits(gbc, 2);           /* layer */
//crc_abs = get_bits1(gbc);    /* protection_absent */
//aot     = get_bits(gbc, 2);  /* profile_objecttype */
//sr      = get_bits(gbc, 4);  /* sample_frequency_index */
//if (!avpriv_mpeg4audio_sample_rates[sr])
//return AAC_AC3_PARSE_ERROR_SAMPLE_RATE;
//skip_bits1(gbc);             /* private_bit */
//ch = get_bits(gbc, 3);       /* channel_configuration */
//
//skip_bits1(gbc);             /* original/copy */
//skip_bits1(gbc);             /* home */
//
///* adts_variable_header */
//skip_bits1(gbc);             /* copyright_identification_bit */
//skip_bits1(gbc);             /* copyright_identification_start */
//size = get_bits(gbc, 13);    /* aac_frame_length */
//if (size < AAC_ADTS_HEADER_SIZE)
//return AAC_AC3_PARSE_ERROR_FRAME_SIZE;
//
//skip_bits(gbc, 11);          /* adts_buffer_fullness */
//rdb = get_bits(gbc, 2);      /* number_of_raw_data_blocks_in_frame */
//
//hdr->object_type    = aot + 1;
//hdr->chan_config    = ch;
//hdr->crc_absent     = crc_abs;
//hdr->num_aac_frames = rdb + 1;
//hdr->sampling_index = sr;
//hdr->sample_rate    = avpriv_mpeg4audio_sample_rates[sr];
//hdr->samples        = (rdb + 1) * 1024;
//hdr->bit_rate       = size * 8 * hdr->sample_rate / hdr->samples;
//
//return size;

int aac_parse_header(uint8_t *adts, int size, int* sampling_frequency_index, int* objType, int* channel_config)
{
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored"-Wunused-variable"
	UINT StartBit = 0;
	UINT* pStartBit = &StartBit;
	int syncword = u(12, adts, &pStartBit);
	if (syncword != 0xfff)return -1;

	int adtsID = u(1, adts, &pStartBit);             /* id */
	int layer = u(2, adts, &pStartBit);           /* layer */
	int protection_absent = u(1, adts, &pStartBit);    /* protection_absent */
	int profile_objecttype = u(2, adts, &pStartBit);   /* profile_objecttype */
	int sample_frequency_index = u(4, adts, &pStartBit);   /* sample_frequency_index */
	if (!mpeg4audio_sample_rates[sample_frequency_index])
		return -1;
	int private_bit = u(1, adts, &pStartBit);             /* private_bit */
	int channel_configuration = u(3, adts, &pStartBit);       /* channel_configuration */

	int original_copy = u(1, adts, &pStartBit);            /* original/copy */
	int home = u(1, adts, &pStartBit);            /* home */

	/* adts_variable_header */
	int copyright_identification_bit = u(1, adts, &pStartBit);             /* copyright_identification_bit */
	int copyright_identification_start = u(1, adts, &pStartBit);             /* copyright_identification_start */
	int aac_frame_length = u(13, adts, &pStartBit);    /* aac_frame_length */
	if (size < 7)return -1;

	int adts_buffer_fullness = u(11, adts, &pStartBit);         /* adts_buffer_fullness */
	int number_of_raw_data_blocks_in_frame = u(2, adts, &pStartBit);      /* number_of_raw_data_blocks_in_frame */

	*sampling_frequency_index = sample_frequency_index;
	*objType = profile_objecttype;
	*channel_config = channel_configuration;

	return size;
//#pragma clang diagnostic pop
}

