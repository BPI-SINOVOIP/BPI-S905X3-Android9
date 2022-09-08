#ifndef __SUBTITLE_SOCKET_CLIENT_API_H__
#define __SUBTITLE_SOCKET_CLIENT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void *SubSourceHandle;

typedef enum {
    SUB_STAT_INV = -1,
    SUB_STAT_FAIL = 0,
    SUB_STAT_OK,
}SubSourceStatus;

SubSourceHandle SubSource_Create(int sId);
SubSourceStatus SubSource_Destroy(SubSourceHandle handle);


SubSourceStatus SubSource_Reset(SubSourceHandle handle);
SubSourceStatus SubSource_Stop(SubSourceHandle handle);

SubSourceStatus SubSource_ReportRenderTime(SubSourceHandle handle, int64_t timeUs);
SubSourceStatus SubSource_ReportStartPts(SubSourceHandle handle, int64_t type);

SubSourceStatus SubSource_ReportTotalTracks(SubSourceHandle handle, int trackNum);
SubSourceStatus SubSource_ReportType(SubSourceHandle handle, int type);

SubSourceStatus SubSource_ReportSubTypeString(SubSourceHandle handle, const char *type);
SubSourceStatus SubSource_ReportLauguageString(SubSourceHandle handle, const char *lang);


SubSourceStatus SubSource_SendData(SubSourceHandle handle, const char *data, int size);


#ifdef __cplusplus
}
#endif


#endif
