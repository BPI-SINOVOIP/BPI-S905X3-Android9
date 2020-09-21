vts_test_host_bin_packages := \
    host_cross_vndk-vtable-dumper \
    trace_processor \
    vndk-vtable-dumper \
    img2simg \
    simg2img \
    mkuserimg_mke2fs.sh \

# Need to package mkdtboimg.py since the tool is not just used by the VTS test.
vts_test_host_bin_packages += \
    mkdtboimg.py \
