#ifndef _CTC_UTILS_H_
#define _CTC_UTILS_H_

#include <cutils/properties.h>
#include <inttypes.h>
#include <CTC_Common.h>

#define RETURN_IF(error, cond)                                                       \
    do {                                                                             \
        if (cond) {                                                                  \
            ALOGE("%s:%d return %s <- %s", __FUNCTION__, __LINE__, #error, #cond);   \
            return error;                                                            \
        }                                                                            \
    } while (0)

#define RETURN_VOID_IF(cond)                                                         \
    do {                                                                             \
        if (cond) {                                                                  \
            ALOGE("%s:%d return <- %s", __FUNCTION__, __LINE__, #cond);              \
            return;                                                                  \
        }                                                                            \
    } while (0)

template <typename T>
T CTC_getConfig(const char* key, const T& defaultValue = T())
{
    char buf[PROP_VALUE_MAX];
    T value;

    if (property_get(key, buf, nullptr) > 0) {
        char* end = nullptr;
        value = strtoimax(buf, &end, 0); 

        if (end == buf) {
            value = defaultValue;
        }
    } else {
        value = defaultValue;
    }

    return value;
}

typedef enum {
    OUTPUT_MODE_480I = 0,
    OUTPUT_MODE_480P,
    OUTPUT_MODE_576I,
    OUTPUT_MODE_576P,
    OUTPUT_MODE_720P,
    OUTPUT_MODE_1080I,
    OUTPUT_MODE_1080P,
    OUTPUT_MODE_4K2K,
    OUTPUT_MODE_4K2KSMPTE,
}OUTPUT_MODE;

OUTPUT_MODE get_display_mode();
void getPosition(OUTPUT_MODE output_mode, int *x, int *y, int *width, int *height);

struct CTC_MediaInfo {
    size_t aParaCount;
    size_t sParaCcount;
    VIDEO_PARA_T vPara;
    AUDIO_PARA_T aParas[MAX_AUDIO_PARAM_SIZE];
    SUBTITLE_PARA_T sParas[MAX_SUBTITLE_PARAM_SIZE];
    int64_t bitrate;
};

int getMediaInfo(const char* url, CTC_MediaInfo* mediaInfo);
void freeMediaInfo(CTC_MediaInfo* mediaInfo);


#endif /* ifndef _CTC_UTILS_H_ */
