#ifndef _MEDIA_PROCESSOR_JNI_H_
#define _MEDIA_PROCESSOR_JNI_H_
#ifdef __cplusplus
extern "C" {
#endif

//从java层获取surface
jint Java_com_ctc_MediaProcessorDemoActivity_nativeCreateSurface(JNIEnv* env, jobject thiz, jobject pSurface, int w, int h, int use_omx_decoder);

//根据音视频参数初始化播放器
jint Java_com_ctc_MediaProcessorDemoActivity_nativeInit(JNIEnv* env, jobject thiz, jstring url, int use_omx_decoder);

//开始播放
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStartPlay(JNIEnv* env, jobject thiz, int use_omx_decoder);

//写入数据播放
jint Java_com_ctc_MediaProcessorDemoActivity_nativeWriteData(JNIEnv* env, jobject thiz, jstring url, jint bufsize, int use_omx_decoder);

//取得播放模式
jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetPlayMode(JNIEnv* env, jobject thiz, int use_omx_decoder);

//设置播放区域的位置和播放区域的宽高
jint Java_com_ctc_MediaProcessorDemoActivity_nativeSetVideoWindow(JNIEnv* env, jobject thiz ,jint x, jint y, jint width, jint height, int use_omx_decoder);

//播放器暂停
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativePause(JNIEnv* env, jobject thiz, int use_omx_decoder);

//播放器继续播放
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeResume(JNIEnv* env, jobject thiz, int use_omx_decoder);

//播放器选时
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSeek(JNIEnv* env, jobject thiz, int use_omx_decoder);

//显示视频
jint Java_com_ctc_MediaProcessorDemoActivity_nativeVideoShow(JNIEnv* env, jobject thiz, int use_omx_decoder);

//隐藏视频
jint Java_com_ctc_MediaProcessorDemoActivity_nativeVideoHide(JNIEnv* env, jobject thiz, int use_omx_decoder);

//快进快退
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeFast(JNIEnv* env, jobject thiz, int use_omx_decoder);

//停止快进快退
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStopFast(JNIEnv* env, jobject thiz, int use_omx_decoder);

//停止播放
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStop(JNIEnv* env, jobject thiz, int use_omx_decoder);

//获取音量
jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetVolume(JNIEnv* env, jobject thiz, int use_omx_decoder);

//设定音量
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetVolume(JNIEnv* env, jobject thiz,jint volume, int use_omx_decoder);

//设定视频显示比例
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetRatio(JNIEnv* env, jobject thiz,jint nRatio, int use_omx_decoder);

//获取当前声道
jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetAudioBalance(JNIEnv* env, jobject thiz, int use_omx_decoder);

//设置声道
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetAudioBalance(JNIEnv* env, jobject thiz, jint nAudioBalance, int use_omx_decoder);

//获取视频分辨率
void Java_com_ctc_MediaProcessorDemoActivity_nativeGetVideoPixels(JNIEnv* env, jobject thiz, int use_omx_decoder);

//获取是否由软件拉伸，如果由硬件拉伸，返回false
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeIsSoftFit(JNIEnv* env, jobject thiz, int use_omx_decoder);

jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetCurrentPlayTime(JNIEnv* env, jobject thiz, int use_omx_decoder);

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSwitchAudioTrack(JNIEnv* env, jobject thiz, int use_omx_decoder);

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeInitSubtitle(JNIEnv* env, jobject thiz, int use_omx_decoder);
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSwitchSubtitle(JNIEnv* env, jobject thiz, jint sub_pid, int use_omx_decoder);

#ifdef __cplusplus
}
#endif
#endif

