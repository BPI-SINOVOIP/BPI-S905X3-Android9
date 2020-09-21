/** @file ScreenControlService.h
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   huijie huang
 *  @version  1.0
 *  @date     2019/05/06
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#ifndef ANDROID_GUI_SCREENCONTROLCLIENT_H
#define ANDROID_GUI_SCREENCONTROLCLIENT_H

#include <utils/Errors.h>
#include <string>
#include <vector>
#include <vendor/amlogic/hardware/screencontrol/1.0/IScreenControl.h>
#include <vendor/amlogic/hardware/screencontrol/1.0/types.h>

using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControl;
using ::vendor::amlogic::hardware::screencontrol::V1_0::Result;
using ::android::hardware::Return;

namespace android {

class ScreenControlClient  : virtual public RefBase {
public:
	ScreenControlClient();

	~ScreenControlClient();

	static ScreenControlClient *getInstance();

	int startScreenRecord(int32_t width, int32_t height, int32_t frameRate,
		int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename);

	int startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom,
		int32_t width, int32_t height, int32_t sourceType, const char* filename);

	int startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
		int32_t width, int32_t height, int32_t sourceType, void **buffer, int *bufSize);

private:
	static ScreenControlClient *mInstance;
	sp<IScreenControl> mScreenCtrl;
};

}
#endif

