/**
* Simplest Librtmp Send 264
*
* 雷霄骅，张晖
* leixiaohua1020@126.com
* zhanghuicuc@gmail.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 本程序用于将内存中的H.264数据推送至RTMP流媒体服务器。
*
*/
#ifndef SPS_DECODE_H
#define SPS_DECODE_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;

//pp是i帧或非i帧,isKey时为i帧,,参数太多，不查找ppSize，耗费时间，请直接相减计算,size包括分隔符
void find_pp_sps_pps(int *isKey, uint8_t* data, int size, uint8_t **pp, uint8_t **sps, int *spsSize, uint8_t** pps, int *ppsSize, uint8_t** sei, int *seiSize);
int h264_decode_sps(BYTE * buf, unsigned int nLen, int* width, int* height, int* fps,int* stride);
int aac_parse_header(uint8_t *adts, int size, int* samples, int* objType, int* channel_config);
#endif