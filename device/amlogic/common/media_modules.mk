
DEFAULT_MEDIA_KERNEL_MODULES := \
	$(PRODUCT_OUT)/obj/lib_vendor/media_clock.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/firmware.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/decoder_common.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/stream_input.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_avs.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_avs2.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mpeg12.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mmpeg12.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mpeg4.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mmpeg4.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_h264.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mh264.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_h264mvc.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_h265.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mjpeg.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mmjpeg.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_vc1.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_vp9.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_real.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/encoder.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/vpu.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/aml_hardware_dmx.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/ci.ko  \
	$(PRODUCT_OUT)/obj/lib_vendor/cimax-usb.ko  \
	$(PRODUCT_OUT)/obj/lib_vendor/amvdec_mavs.ko

include hardware/amlogic/media_modules/Media.mk
