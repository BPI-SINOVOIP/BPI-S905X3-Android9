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

LOCAL_MODULE := MagickCore

LOCAL_SDK_VERSION := 24

LOCAL_SRC_FILES := accelerate.c\
    client.c\
    decorate.c\
    feature.c\
    linked-list.c\
    monitor.c\
    prepress.c\
    resize.c\
    string.c\
    vms.c\
    animate.c\
    coder.c\
    delegate.c\
    fourier.c\
    list.c\
    montage.c\
    profile.c\
    resource.c\
    thread.c\
    widget.c\
    annotate.c\
    color.c\
    deprecate.c\
    fx.c\
    locale.c\
    morphology.c\
    property.c\
    segment.c\
    threshold.c\
    xml-tree.c\
    artifact.c\
    colormap.c\
    display.c\
    gem.c\
    log.c\
    nt-base.c\
    quantize.c\
    semaphore.c\
    timer.c\
    xwindow.c\
    attribute.c\
    colorspace.c\
    distort.c\
    geometry.c\
    magic.c\
    nt-feature.c\
    quantum-export.c\
    shear.c\
    token.c\
    blob.c\
    compare.c\
    distribute-cache.c\
    histogram.c\
    magick.c\
    opencl.c\
    quantum-import.c\
    signature.c\
    transform.c\
    cache-view.c\
    composite.c\
    draw.c\
    identify.c\
    matrix.c\
    option.c\
    quantum.c\
    splay-tree.c\
    type.c\
    cache.c\
    compress.c\
    effect.c\
    image-view.c\
    memory.c\
    paint.c\
    random.c\
    static.c\
    utility.c\
    channel.c\
    configure.c\
    enhance.c\
    image.c\
    mime.c\
    pixel.c\
    registry.c\
    statistic.c\
    version.c\
    cipher.c\
    constitute.c\
    exception.c\
    layer.c\
    module.c\
    policy.c\
    resample.c\
    stream.c\
    vision.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/.. \
    external/freetype/include

LOCAL_CFLAGS += -DHAVE_CONFIG_H \
    -Wall -Werror \
    -Wno-deprecated-register \
    -Wno-enum-conversion \
    -Wno-for-loop-analysis \
    -Wno-unused-function \
    -Wno-unused-parameter \

LOCAL_STATIC_LIBRARIES += libbz
LOCAL_SHARED_LIBRARIES += libft2 liblzma libxml2 libicuuc libpng libjpeg

include $(BUILD_STATIC_LIBRARY)
