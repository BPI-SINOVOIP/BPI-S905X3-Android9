/*
 * Copyright (C) 2010 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#include <cutils/properties.h>
//#include <gui/SurfaceTextureClient.h>
#include <media/ICrypto.h>

#define LOG_TAG "MediaConvertorTest"


#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AHandler.h>

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>

//#include <MetadataBufferType.h>

#include <ui/GraphicBuffer.h>
#include <gui/ISurfaceComposer.h>
//#include <gui/IGraphicBufferAlloc.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>

#include "esconvertor.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/videodev2.h>
#include <hardware/hardware.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "../ScreenControlService.h"

int main(int argc, char **argv) {

    using namespace android;
    int err;
    int isAudio;
    int dump_time = 0, video_dump_size = 0, audio_dump_size = 0;
    MediaBuffer *tVideoBuffer, *tAudioBuffer;

    ProcessState::self()->startThreadPool();

    int  audio_file   = open("/data/temp/audio.es", O_CREAT | O_RDWR, 0666);

    sp<ESConvertor> mAACConvertor = new ESConvertor(AML_CAPTURE_OSD_VIDEO, 1);

    sp<MetaData> params_audio = new MetaData;
    params_audio->setInt32(kKeyChannelCount, 2);
    params_audio->setInt32(kKeySampleRate, 48000);
    params_audio->setInt32(kKeyIsADTS, 1);

    err = mAACConvertor->start(params_audio.get());
    if (err != OK) {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        return OK;
    }

    while (1) {
        tAudioBuffer = NULL;
        err = mAACConvertor->read(&tAudioBuffer);

        if (err != OK) {
            usleep(1);
            continue;
        }

        write(audio_file, tAudioBuffer->data(), tAudioBuffer->range_length());
        audio_dump_size += tAudioBuffer->range_length();
        ALOGE("audio dump_time:%d size:%d dump_size:%d\n", dump_time, tAudioBuffer->range_length(), audio_dump_size);
        dump_time++;

        tAudioBuffer->release();
        tAudioBuffer = NULL;

        if (dump_time > 500)
            break;
    }

    ALOGE("mH264Convertor stop\n");

    mAACConvertor->stop();
    close(audio_file);

    ALOGE("file close\n");
    return 0;
}
