#ifndef _CTC_LOG_H
#define _CTC_LOG_H

#include <utils/Log.h>

extern int prop_shouldshowlog;

#define ENABLE_CUSTOM_LOG_TAG   0

#if ENABLE_CUSTOM_LOG_TAG
#if LOG_NDEBUG
#define MLOGV(fmt, ...)
#else
#define MLOGV(fmt, ...) ALOG(LOG_VERBOSE, mName, fmt, ##__VA_ARGS__)
#endif

#define MLOGD(fmt, ...) ({prop_shouldshowlog ? ALOG(LOG_DEBUG, mName, fmt, ##__VA_ARGS__) : 0 ;})
#define MLOGI(fmt, ...) ALOG(LOG_INFO, mName, fmt, ##__VA_ARGS__)
#define MLOGW(fmt, ...) ALOG(LOG_WARN, mName, fmt, ##__VA_ARGS__)
#define MLOGE(fmt, ...) ALOG(LOG_ERROR, mName, fmt, ##__VA_ARGS__)
#else
#if LOG_NDEBUG
#define MLOGV(fmt, ...)
#else
#define MLOGV(fmt, ...) ALOG(LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
#endif

#define MLOGD(fmt, ...) ({prop_shouldshowlog ? ALOG(LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__) : 0 ;})
#define MLOGI(fmt, ...) ALOG(LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define MLOGW(fmt, ...) ALOG(LOG_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#define MLOGE(fmt, ...) ALOG(LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)
#endif

#ifndef MLOG
#define MLOG(fmt, ...) MLOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef ALOGV
#undef ALOGV
#endif
#define ALOGV MLOGV

#ifdef ALOGD
#undef ALOGD
#endif
#define ALOGD MLOGD

#ifdef ALOGI
#undef ALOGI
#endif
#define ALOGI MLOGI


#ifdef ALOGW
#undef ALOGW
#endif
#define ALOGW MLOGW


#ifdef ALOGE
#undef ALOGE
#endif
#define ALOGE MLOGE

#ifdef LOGV
#undef LOGV
#endif
#define LOGV MLOGV

#ifdef LOGD
#undef LOGD
#endif
#define LOGD MLOGD

#ifdef LOGI
#undef LOGI
#endif
#define LOGI MLOGI


#ifdef LOGW
#undef LOGW
#endif
#define LOGW MLOGW


#ifdef LOGE
#undef LOGE
#endif
#define LOGE MLOGE


#endif /* ifndef _CTC_LOG_H */

