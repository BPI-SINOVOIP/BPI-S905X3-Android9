/*
 * author: wei.liu@amlogic.com
 * date: 2012-07-12
 * wrap original source code for CTC usage
 */

#ifndef _CTC_MEDIACONTROL_H_
#define _CTC_MEDIACONTROL_H_
#include "CTsPlayer.h"

class CTC_MediaControl;

// 获取CTC_MediaControl 派生类的实例对象。在GetMediaControl () 这个接口的实现中，需要创建一个
// CTC_MediaContro 派生类的实例，然后返回这个实例的指针
ITsPlayer* GetMediaControl(int use_omx_decoder);  // { return NULL; }

CTC_MediaControl* GetMediaControlImpl();
ITsPlayer* GetMediaControl();

// 获取底层模块实现的接口版本号。将来如果有多个底层接口定义，使得上层与底层之间能够匹配。本版本定义
// 返回为1
int Get_MediaControlVersion();  //  { return 0; }

#endif  // _CTC_MEDIACONTROL_H_
