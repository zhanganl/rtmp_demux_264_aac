#ifndef FLVPARSER_H
#define FLVPARSER_H

#include <fstream>
#include <vector>

#include "Videojj.h"

using namespace std;

#define SSRC_NUM                    10
#define kRtpMarkerBitMask			0x80



//typedef unsigned long long uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef __int64             int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned __int64    uint64_t;

typedef struct rtp_header {
	/* byte 1 */
	uint8_t first_byte;
	uint8_t payload_type;
	/* bytes 2, 3 */
	uint16_t seq_no;
	/* bytes 4-7 */
	uint32_t timestamp;
	/* bytes 8-11 */
	uint32_t ssrc;
}rtp_header_t; /* 12 bytes */

class CFlvParser
{
public:
	CFlvParser();
	virtual ~CFlvParser();

	int Parse(unsigned char *pBuf, int nBufSize, int &nUsedLen);
	int PrintInfo();
	int DumpH264();
	int DumpAAC();

private:
	typedef struct FlvHeader_s
	{
		int nVersion;
		int bHaveVideo, bHaveAudio;
		int nHeadSize;

		unsigned char *pFlvHeader;
	} FlvHeader;
	typedef struct TagHeader_s
	{
		int nType;
		int nDataSize;
		int nTimeStamp;
		int nTSEx;
		int nStreamID;

		unsigned int nTotalTS;
	} TagHeader;

	class Tag
	{
	public:
		void Init(TagHeader *pHeader, unsigned char *pBuf, int nLeftLen);

		TagHeader _header;
		unsigned char *_pTagData;
		unsigned char *_pMedia;
		int _nMediaLen;
	};

	class CVideoTag : public Tag
	{
	public:
		CVideoTag(TagHeader *pHeader, unsigned char *pBuf, int nLeftLen, CFlvParser *pParser);

		int _nFrameType;
		int _nCodecID;
		int ParseH264Tag(CFlvParser *pParser);
		int ParseH264Configuration(CFlvParser *pParser, unsigned char *pTagData);
		int ParseNalu(CFlvParser *pParser, unsigned char *pTagData);
	};

	class CAudioTag : public Tag
	{
	public:
		CAudioTag(TagHeader *pHeader, unsigned char *pBuf, int nLeftLen, CFlvParser *pParser);

		int _nSoundFormat;
		int _nSoundRate;
		int _nSoundSize;
		int _nSoundType;

		// aac
		static int _aacProfile;
		static int _sampleRateIndex;
		static int _channelConfig;

		int ParseAACTag(CFlvParser *pParser);
		int ParseAudioSpecificConfig(CFlvParser *pParser, unsigned char *pTagData);
		int ParseRawAAC(CFlvParser *pParser, unsigned char *pTagData);
	};

	typedef struct FlvStat_s
	{
		int nMetaNum, nVideoNum, nAudioNum;
		int nMaxTimeStamp;
		int nLengthSize = 0;
	} FlvStat;

	int stream2rtp(int b_video, uint8_t *SrcStream, int Srclen, uint8_t *DstStream, int &Dstlen, bool marker_bit, uint32_t timestamp_rtp);

	static unsigned int ShowU32(unsigned char *pBuf) { return (pBuf[0] << 24) + (pBuf[1] << 16) + (pBuf[2] << 8) + pBuf[3]; }
	static unsigned int ShowU24(unsigned char *pBuf) { return (pBuf[0] << 16) + (pBuf[1] << 8) + (pBuf[2]); }
	static unsigned int ShowU16(unsigned char *pBuf) { return (pBuf[0] << 8) + (pBuf[1]); }
	static unsigned int ShowU8(unsigned char *pBuf) { return (pBuf[0]); }
	static void WriteU64(uint64_t & x, int length, int value)
	{
		uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - length);
		x = (x << length) | ((uint64_t)value & mask);
	}

	friend Tag;
	
private:
	fstream f_v;
	fstream f_a;

	FlvHeader *CreateFlvHeader(unsigned char *pBuf);
	int DestroyFlvHeader(FlvHeader *pHeader);
	Tag *CreateTag(unsigned char *pBuf, int nLeftLen);
	int DestroyTag(Tag *pTag);
	int Stat();
	int StatVideo(Tag *pTag);
	int IsUserDataTag(Tag *pTag);

private:
	uint16_t sequence_number_v;
	uint16_t sequence_number_a;
	uint32_t ts_current_v;
	uint32_t ts_current_a;

	FlvHeader* _pFlvHeader;
	vector<Tag *> _vpTag;
	FlvStat _sStat;
	CVideojj *_vjj;

	// H.264
	int _nNalUnitLength;

};

#endif // FLVPARSER_H
