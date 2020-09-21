#ifndef _MEDIA_PROCESSOR_JNI_H_
#define _MEDIA_PROCESSOR_JNI_H_
#ifdef __cplusplus
extern "C" {
#endif

//��java���ȡsurface
jint Java_com_ctc_MediaProcessorDemoActivity_nativeCreateSurface(JNIEnv* env, jobject thiz, jobject pSurface, int w, int h, int use_omx_decoder);

//��������Ƶ������ʼ��������
jint Java_com_ctc_MediaProcessorDemoActivity_nativeInit(JNIEnv* env, jobject thiz, jstring url, int use_omx_decoder);

//��ʼ����
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStartPlay(JNIEnv* env, jobject thiz, int use_omx_decoder);

//д�����ݲ���
jint Java_com_ctc_MediaProcessorDemoActivity_nativeWriteData(JNIEnv* env, jobject thiz, jstring url, jint bufsize, int use_omx_decoder);

//ȡ�ò���ģʽ
jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetPlayMode(JNIEnv* env, jobject thiz, int use_omx_decoder);

//���ò��������λ�úͲ�������Ŀ��
jint Java_com_ctc_MediaProcessorDemoActivity_nativeSetVideoWindow(JNIEnv* env, jobject thiz ,jint x, jint y, jint width, jint height, int use_omx_decoder);

//��������ͣ
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativePause(JNIEnv* env, jobject thiz, int use_omx_decoder);

//��������������
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeResume(JNIEnv* env, jobject thiz, int use_omx_decoder);

//������ѡʱ
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSeek(JNIEnv* env, jobject thiz, int use_omx_decoder);

//��ʾ��Ƶ
jint Java_com_ctc_MediaProcessorDemoActivity_nativeVideoShow(JNIEnv* env, jobject thiz, int use_omx_decoder);

//������Ƶ
jint Java_com_ctc_MediaProcessorDemoActivity_nativeVideoHide(JNIEnv* env, jobject thiz, int use_omx_decoder);

//�������
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeFast(JNIEnv* env, jobject thiz, int use_omx_decoder);

//ֹͣ�������
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStopFast(JNIEnv* env, jobject thiz, int use_omx_decoder);

//ֹͣ����
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStop(JNIEnv* env, jobject thiz, int use_omx_decoder);

//��ȡ����
jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetVolume(JNIEnv* env, jobject thiz, int use_omx_decoder);

//�趨����
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetVolume(JNIEnv* env, jobject thiz,jint volume, int use_omx_decoder);

//�趨��Ƶ��ʾ����
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetRatio(JNIEnv* env, jobject thiz,jint nRatio, int use_omx_decoder);

//��ȡ��ǰ����
jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetAudioBalance(JNIEnv* env, jobject thiz, int use_omx_decoder);

//��������
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetAudioBalance(JNIEnv* env, jobject thiz, jint nAudioBalance, int use_omx_decoder);

//��ȡ��Ƶ�ֱ���
void Java_com_ctc_MediaProcessorDemoActivity_nativeGetVideoPixels(JNIEnv* env, jobject thiz, int use_omx_decoder);

//��ȡ�Ƿ���������죬�����Ӳ�����죬����false
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeIsSoftFit(JNIEnv* env, jobject thiz, int use_omx_decoder);

jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetCurrentPlayTime(JNIEnv* env, jobject thiz, int use_omx_decoder);

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSwitchAudioTrack(JNIEnv* env, jobject thiz, int use_omx_decoder);

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeInitSubtitle(JNIEnv* env, jobject thiz, int use_omx_decoder);
jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSwitchSubtitle(JNIEnv* env, jobject thiz, jint sub_pid, int use_omx_decoder);

#ifdef __cplusplus
}
#endif
#endif

