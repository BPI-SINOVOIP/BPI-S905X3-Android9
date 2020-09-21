/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.util.Log;

public class TvChannelSetting {
    private static String TAG = "TvChannelSetting";

    /* format:
     * format & 0xff000000: PAL/NTSC/SECAM in atv demod and tuner.
     * format & 0x00ffffff: CVBS format in atv decoder.
     *
     * mode:
     * PAL = 1,
     * NTSC = 2,
     * SECAM = 3,
     */
    private static int convertAtvCvbsFormat(int format, int mode) {
        switch (mode) {
            case TvControlManager.ATV_VIDEO_STD_PAL:
                return ((format & 0x00FFFFFF) | TvControlManager.V4L2_COLOR_STD_PAL);
            case TvControlManager.ATV_VIDEO_STD_NTSC:
                return ((format & 0x00FFFFFF) | TvControlManager.V4L2_COLOR_STD_NTSC);
            case TvControlManager.ATV_VIDEO_STD_SECAM:
                return ((format & 0x00FFFFFF) | TvControlManager.V4L2_COLOR_STD_SECAM);
            default:
                Log.e(TAG, "convertAtvCvbsFormat error mode == " + mode);
                break;
        }

        return format;
    }

    /* video mode:
     * PAL = 1,
     * NTSC = 2,
     * SECAM = 3,
     */
    public static void setAtvChannelVideo(ChannelInfo channel, int mode) {
        if (channel != null) {
            channel.setVideoStd(mode);
            channel.setVfmt(convertAtvCvbsFormat(channel.getVfmt(), mode));

            Log.d(TAG, "setAtvChannelVideo mode: " + mode);

            TvControlManager.getInstance().SetFrontendParms(
                    TvControlManager.tv_fe_type_e.TV_FE_ANALOG,
                    channel.getFrequency() + channel.getFineTune(),
                    channel.getVideoStd(),
                    channel.getAudioStd(),
                    channel.getVfmt(),
                    channel.getAudioOutPutMode(),
                    0,
                    0);
        } else {
            Log.e(TAG, "setAtvChannelVideo error channel == null");
        }
    }

    /* audio mode:
     * DK = 0,
     * I = 1,
     * BG = 2,
     * M = 3,
     * L = 4,
     */
    public static void setAtvChannelAudio(ChannelInfo channel, int mode) {
        if (channel != null) {
            channel.setAudioStd(mode);

            Log.d(TAG, "setAtvChannelAudio mode: " + mode);

            TvControlManager.getInstance().SetFrontendParms(
                    TvControlManager.tv_fe_type_e.TV_FE_ANALOG,
                    channel.getFrequency() + channel.getFineTune(),
                    channel.getVideoStd(),
                    channel.getAudioStd(),
                    channel.getVfmt(),
                    channel.getAudioOutPutMode(),
                    0,
                    0);
        } else {
            Log.e(TAG, "setAtvChannelAudio error channel == null");
        }
    }

    public static void setAtvFineTune(ChannelInfo channel, int finetune) {
        if (channel != null) {
            channel.setFinetune(finetune);

            Log.d(TAG, "setAtvFineTune finetune: " + finetune);

            TvControlManager.getInstance().SetFrontendParms(
                    TvControlManager.tv_fe_type_e.TV_FE_ANALOG,
                    channel.getFrequency() + channel.getFineTune(),
                    channel.getVideoStd(),
                    channel.getAudioStd(),
                    channel.getVfmt(),
                    channel.getAudioOutPutMode(),
                    0,
                    0);
        } else {
            Log.e(TAG, "setAtvFineTune error channel == null");
        }
    }
}
