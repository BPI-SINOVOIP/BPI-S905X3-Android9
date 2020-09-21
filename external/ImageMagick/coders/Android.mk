# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := Magick_coders

LOCAL_SDK_VERSION := 24

LOCAL_SRC_FILES += aai.c\
    art.c\
    avs.c\
    bgr.c\
    bmp.c\
    braille.c\
    cals.c\
    caption.c\
    cin.c\
    cip.c\
    clip.c\
    clipboard.c\
    cmyk.c\
    cut.c\
    dcm.c\
    dds.c\
    debug.c\
    dib.c\
    djvu.c\
    dng.c\
    dot.c\
    dps.c\
    dpx.c\
    emf.c\
    ept.c\
    exr.c\
    fax.c\
    fd.c\
    fits.c\
    flif.c\
    fpx.c\
    gif.c\
    gradient.c\
    gray.c\
    hald.c\
    hdr.c\
    histogram.c\
    hrz.c\
    html.c\
    icon.c\
    info.c\
    inline.c\
    ipl.c\
    jbig.c\
    jnx.c\
    jp2.c\
    jpeg.c\
    json.c\
    label.c\
    mac.c\
    magick.c\
    map.c\
    mask.c\
    mat.c\
    matte.c\
    meta.c\
    mono.c\
    mpc.c\
    mpeg.c\
    mpr.c\
    mtv.c\
    mvg.c\
    null.c\
    otb.c\
    palm.c\
    pango.c\
    pattern.c\
    pcd.c\
    pcl.c\
    pcx.c\
    pdb.c\
    pdf.c\
    pes.c\
    pict.c\
    pix.c\
    plasma.c\
    png.c\
    pnm.c\
    ps.c\
    ps2.c\
    ps3.c\
    psd.c\
    pwp.c\
    raw.c\
    rgb.c\
    rgf.c\
    rla.c\
    rle.c\
    scr.c\
    screenshot.c\
    sct.c\
    sfw.c\
    sgi.c\
    sixel.c\
    stegano.c\
    sun.c\
    tga.c\
    thumbnail.c\
    tile.c\
    tim.c\
    ttf.c\
    txt.c\
    uil.c\
    url.c\
    uyvy.c\
    vicar.c\
    vid.c\
    viff.c\
    vips.c\
    wbmp.c\
    webp.c\
    wmf.c\
    wpg.c\
    x.c\
    xbm.c\
    xc.c\
    xcf.c\
    xpm.c\
    xps.c\
    xtrn.c\
    xwd.c\
    ycbcr.c\
    msl.c\
    svg.c\
    yuv.c\
    miff.c
    #tiff.c\ # Removed because requires LIBTIFF

LOCAL_C_INCLUDES += $(LOCAL_PATH)/..

LOCAL_CFLAGS += -DHAVE_CONFIG_H \
    -Wall -Werror \
    -Wno-for-loop-analysis \
    -Wno-sign-compare \
    -Wno-unused-function \
    -Wno-unused-parameter

LOCAL_STATIC_LIBRARIES += libbz
LOCAL_SHARED_LIBRARIES += libft2 liblzma libxml2 libicuuc libpng libjpeg

include $(BUILD_STATIC_LIBRARY)
