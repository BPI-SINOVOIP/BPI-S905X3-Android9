#ifndef FF_EXTRACTOR
#define FF_EXTRACTOR
#include <stdio.h>

#define AV_PKT_FLAG_KEY 0x0001
#define AV_PKT_FLAG_CORRUPT 0x0002

typedef enum MEDIA_TYPE {
	UNKNOWN = -1,
	AUDIO=0,
	VIDEO,
	SUBTITLE
} MEDIA_TYPE;


typedef struct {
	void *data;
	unsigned size;
	int64_t pts;
	int64_t duration;
	int flags;
	MEDIA_TYPE TYPE;
}MediaPacket;

typedef struct{
        unsigned short  pid;//pid
        int width;//视频宽度
        int height;//视频高度
        int framerate;//帧率
        unsigned long   cFmt;//编码格式
}MediaInfo;

typedef struct {
	void *data;
	size_t size;
	size_t width;
	size_t height;
	int64_t pts;
	uint32_t flags;
}YUVFrame;
int ffextractor_init(int(*read_cb)(void *opaque, uint8_t *buf, int size), MediaInfo *pMi);
void ffextractor_read_packet(size_t *queue_size);
YUVFrame *ff_decode_frame();
void ffextractor_deinit();
int64_t getCurrentTimeMs();
int64_t getCurrentTimeUs();
int checkLogMask();
#endif
