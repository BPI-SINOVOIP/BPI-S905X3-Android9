#ifndef HW_EMULATOR_CAMERA3_MIPISENSOR_H
#define HW_EMULATOR_CAMERA3_MIPISENSOR_H

#include "Sensor.h"
#include "CameraIO.h"
#include "CameraUtil.h"

namespace android {

	class MIPISensor:public Sensor {
		public:
			MIPISensor();
			~MIPISensor();
		public:
			status_t streamOff(void) override;
			status_t startUp(int idx) override;
			status_t shutDown(void) override;
			void captureRGB(uint8_t *img, uint32_t gain, uint32_t stride) override;
			void captureNV21(StreamBuffer b, uint32_t gain) override;
			void captureYV12(StreamBuffer b, uint32_t gain) override;
			void captureYUYV(uint8_t *img, uint32_t gain, uint32_t stride) override;
			status_t getOutputFormat(void) override;
			status_t setOutputFormat(int width, int height, int pixelformat, bool isjpeg) override;
			int halFormatToSensorFormat(uint32_t pixelfmt) override;
			status_t IoctlStateProbe(void) override;
			status_t streamOn() override;
			bool isStreaming() override;
			bool isNeedRestart(uint32_t width, uint32_t height, uint32_t pixelformat) override;
			int getStreamConfigurations(uint32_t picSizes[], const int32_t kAvailableFormats[], int size) override;
			int getStreamConfigurationDurations(uint32_t picSizes[], int64_t duration[], int size, bool flag) override;
			int64_t getMinFrameDuration() override;
			int getPictureSizes(int32_t picSizes[], int size, bool preview) override;
			status_t force_reset_sensor() override;
			int captureNewImage() override;
			//-------dummy function-------
			int getZoom(int *zoomMin, int *zoomMax, int *zoomStep) override;
			int setZoom(int zoomValue) override;
			status_t setEffect(uint8_t effect) override;
			int getExposure(int *maxExp, int *minExp, int *def, camera_metadata_rational *step) override;
			status_t setExposure(int expCmp) override;
			int getAntiBanding(uint8_t *antiBanding, uint8_t maxCont) override;
			status_t setAntiBanding(uint8_t antiBanding) override;
			status_t setFocuasArea(int32_t x0, int32_t y0, int32_t x1, int32_t y1) override;
			int getAutoFocus(uint8_t *afMode, uint8_t maxCount) override;
			status_t setAutoFocuas(uint8_t afMode) override;
			int getAWB(uint8_t *awbMode, uint8_t maxCount) override;
			status_t setAWB(uint8_t awbMode) override;
			void setSensorListener(SensorListener *listener) override;
			uint32_t getStreamUsage(int stream_type) override;
		private:
			CameraVirtualDevice* mCameraVirtualDevice;
			int mMIPIDevicefd[3];
			//store the v4l2 info
			CVideoInfo *mVinfo;
			CameraUtil* mCameraUtil;
			uint8_t* mImage_buffer;
			const int MAX_LEVEL_FOR_EXPOSURE = 16;
			const int MIN_LEVEL_FOR_EXPOSURE = 3;
		private:
			int camera_open(int idx);
			void camera_close(void);
			void InitVideoInfo(int idx);
			int SensorInit(int idx);
	};

}
#endif
