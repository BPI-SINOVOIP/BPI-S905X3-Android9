#ifndef AMVIDEOINFO_H
#define AMVIDEOINFO_H

#ifdef  __cplusplus
extern "C" {
#endif
#if 1
typedef enum {
    VID_FRAME_TYPE_UNKNOWN = 0,
    VID_FRAME_TYPE_I,
    VID_FRAME_TYPE_P,
    VID_FRAME_TYPE_B,
    VID_FRAME_TYPE_IDR,
    VID_FRAME_TYPE_BUTT,
}vid_frame_type_x;

typedef struct vid_frame_info{
    vid_frame_type_x enVidFrmType;
    int  nVidFrmSize;
    int  nMinQP;
    int  nMaxQP;
    int  nAvgQP;
    int  nMaxMV;
    int  nMinMV;
    int  nAvgMV;
    int  SkipRatio;
    int  nUnderflow;
}vid_frame_info_x;
#endif

extern int amvideo_ReportVideoFrameInfo(struct vframe_qos_s * pframe_qos);
extern void amvideo_updateframeinfo(struct av_param_info_t av_param_info, struct av_param_qosinfo_t av_param_qosinfo);
extern struct vid_frame_info* codec_get_frame_info(void);
extern void amvideo_checkunderflow_type(codec_para_t  *vcodec);


#ifdef  __cplusplus
}
#endif

#endif

