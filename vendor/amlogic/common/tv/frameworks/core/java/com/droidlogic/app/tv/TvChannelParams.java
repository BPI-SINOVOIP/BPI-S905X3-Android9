/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import java.lang.UnsupportedOperationException;

import android.os.Parcel;
import android.os.Parcelable;
import android.database.Cursor;
import android.content.Context;
import android.util.Log;

public class TvChannelParams  implements Parcelable {
    private static String TAG = "TvChannelParams";
    public static final int TV_FE_HAS_SIGNAL   = 0x01;
    public static final int TV_FE_HAS_CARRIER  = 0x02;
    public static final int TV_FE_HAS_VITERBI  = 0x04;
    public static final int TV_FE_HAS_SYNC     = 0x08;
    public static final int TV_FE_HAS_LOCK     = 0x10;
    public static final int TV_FE_TIMEDOUT     = 0x20;
    public static final int TV_FE_REINIT       = 0x40;

    public static final int MODE_QPSK = 0;

    public static final int MODE_QAM  = 1;

    public static final int MODE_OFDM = 2;

    public static final int MODE_ATSC = 3;

    public static final int MODE_ANALOG = 4;

    public static final int MODE_DTMB = 5;

    public static final int MODE_ISDBT = 6;

    public static final int OFDM_MODE_DVBT=0;
    public static final int OFDM_MODE_DVBT2=1;


    public static int getModeFromString(String str){
        if (str.equals("dvbt"))
            return MODE_OFDM;
        else if (str.equals("dvbc"))
            return MODE_QAM;
        else if (str.equals("dvbs"))
            return MODE_QPSK;
        else if (str.equals("atsc"))
            return MODE_ATSC;
        else if (str.equals("analog"))
            return MODE_ANALOG;
        else if (str.equals("dtmb"))
            return MODE_DTMB;

        return -1;
    }


    public static final int BANDWIDTH_8_MHZ = 0;

    public static final int BANDWIDTH_7_MHZ = 1;

    public static final int BANDWIDTH_6_MHZ = 2;

    public static final int BANDWIDTH_AUTO  = 3;

    public static final int BANDWIDTH_5_MHZ = 4;

    public static final int BANDWIDTH_10_MHZ = 5;


    public static final int MODULATION_QPSK    = 0;

    public static final int MODULATION_QAM_16  = 1;

    public static final int MODULATION_QAM_32  = 2;

    public static final int MODULATION_QAM_64  = 3;

    public static final int MODULATION_QAM_128 = 4;

    public static final int MODULATION_QAM_256 = 5;

    public static final int MODULATION_QAM_AUTO= 6;

    public static final int MODULATION_VSB_8   = 7;

    public static final int MODULATION_VSB_16  = 8;

    public static final int MODULATION_PSK_8   = 9;

    public static final int MODULATION_APSK_16 = 10;

    public static final int MODULATION_APSK_32 = 11;

    public static final int MODULATION_DQPSK   = 12;


    public static final int AUDIO_MONO   = 0x0000;

    public static final int AUDIO_STEREO = 0x0001;

    public static final int AUDIO_LANG2  = 0x0002;
    /**SAP*/
    public static final int AUDIO_SAP    = 0x0002;

    public static final int AUDIO_LANG1  = 0x0003;

    public static final int AUDIO_LANG1_LANG2 = 0x0004;

    /**PAL B*/
    public static final int STD_PAL_B     = 0x00000001;
    /**PAL B1*/
    public static final int STD_PAL_B1    = 0x00000002;
    /**PAL G*/
    public static final int STD_PAL_G     = 0x00000004;
    /**PAL H*/
    public static final int STD_PAL_H     = 0x00000008;
    /**PAL I*/
    public static final int STD_PAL_I     = 0x00000010;
    /**PAL D*/
    public static final int STD_PAL_D     = 0x00000020;
    /**PAL D1*/
    public static final int STD_PAL_D1    = 0x00000040;
    /**PAL K*/
    public static final int STD_PAL_K     = 0x00000080;
    /**PAL M*/
    public static final int STD_PAL_M     = 0x00000100;
    /**PAL N*/
    public static final int STD_PAL_N     = 0x00000200;
    /**PAL Nc*/
    public static final int STD_PAL_Nc    = 0x00000400;
    /**PAL 60*/
    public static final int STD_PAL_60    = 0x00000800;
    /**NTSC M*/
    public static final int STD_NTSC_M    = 0x00001000;
    /**NTSC M JP*/
    public static final int STD_NTSC_M_JP = 0x00002000;
    /**NTSC 443*/
    public static final int STD_NTSC_443  = 0x00004000;
    /**NTSC M KR*/
    public static final int STD_NTSC_M_KR = 0x00008000;
    /**SECAM B*/
    public static final int STD_SECAM_B   = 0x00010000;
    /**SECAM D*/
    public static final int STD_SECAM_D   = 0x00020000;
    /**SECAM G*/
    public static final int STD_SECAM_G   = 0x00040000;
    /**SECAM H*/
    public static final int STD_SECAM_H   = 0x00080000;
    /**SECAM K*/
    public static final int STD_SECAM_K   = 0x00100000;
    /**SECAM K1*/
    public static final int STD_SECAM_K1  = 0x00200000;
    /**SECAM L*/
    public static final int STD_SECAM_L   = 0x00400000;
    /**SECAM LC*/
    public static final int STD_SECAM_LC  = 0x00800000;
    /**ATSC VSB8*/
    public static final int STD_ATSC_8_VSB  = 0x01000000;
    /**ATSC VSB16*/
    public static final int STD_ATSC_16_VSB = 0x02000000;
    /**NTSC*/
    public static final int STD_NTSC      = STD_NTSC_M|STD_NTSC_M_JP|STD_NTSC_M_KR;
    /**SECAM DK*/
    public static final int STD_SECAM_DK  = STD_SECAM_D|STD_SECAM_K|STD_SECAM_K1;
    /**SECAM*/
    public static final int STD_SECAM     = STD_SECAM_B|STD_SECAM_G|STD_SECAM_H|STD_SECAM_DK|STD_SECAM_L|STD_SECAM_LC;
    /**PAL BG*/
    public static final int STD_PAL_BG    = STD_PAL_B|STD_PAL_B1|STD_PAL_G;
    /**PAL DK*/
    public static final int STD_PAL_DK    = STD_PAL_D|STD_PAL_D1|STD_PAL_K;
    /**PAL*/
    public static final int STD_PAL       = STD_PAL_BG|STD_PAL_DK|STD_PAL_H|STD_PAL_I;

    public static final int TUNER_STD_MN  =STD_PAL_M|STD_PAL_N|STD_PAL_Nc| STD_NTSC;
    public static final int STD_B         = STD_PAL_B    |STD_PAL_B1   |STD_SECAM_B;
    public static final int STD_GH        = STD_PAL_G    | STD_PAL_H   |STD_SECAM_G  |STD_SECAM_H;
    public static final int STD_DK        = STD_PAL_DK   | STD_SECAM_DK;
    public static final int STD_M         = STD_PAL_M    | STD_NTSC_M;
    public static final int STD_BG        = STD_PAL_BG   | STD_SECAM_B | STD_SECAM_G ;

    public static final int COLOR_AUTO  =0x02000000;
    public static final int COLOR_PAL   =0x04000000;
    public static final int COLOR_NTSC  =0x08000000;
    public static final int COLOR_SECAM =0x10000000;

    public static final int SAT_POLARISATION_H = 0;

    public static final int SAT_POLARISATION_V = 1;

    public static final int ISDBT_LAYER_ALL = 0;
    /**ISDBT LAYER A*/
    public static final int ISDBT_LAYER_A = 0;
    /**ISDBT LAYER B*/
    public static final int ISDBT_LAYER_B = 0;
    /**ISDBT LAYER C*/
    public static final int ISDBT_LAYER_C = 0;

    public int mode;
    public int frequency;
    public int symbolRate;
    public int modulation;
    public int bandwidth;
    public int ofdm_mode;
    public int audio;
    public int standard;
    public int afc_data;
    public int sat_id;
    public TvSatelliteParams tv_satparams;
    public int sat_polarisation;
    public int isdbtLayer;

    public static final Parcelable.Creator<TvChannelParams> CREATOR = new Parcelable.Creator<TvChannelParams>(){
        public TvChannelParams createFromParcel(Parcel in) {
            return new TvChannelParams(in);
        }
        public TvChannelParams[] newArray(int size) {
            return new TvChannelParams[size];
        }
    };

    public void readFromParcel(Parcel in){
        mode      = in.readInt();
        frequency = in.readInt();
        if ((mode == MODE_QAM) || (mode == MODE_QPSK))
            symbolRate = in.readInt();
        if ((mode == MODE_QAM) || (mode == MODE_ATSC)) {
            modulation = in.readInt();
        }
        if (mode == MODE_OFDM || mode == MODE_DTMB) {
            bandwidth = in.readInt();
            ofdm_mode = in.readInt();
        }
        if (mode == MODE_ANALOG) {
            audio = in.readInt();
            standard = in.readInt();
            afc_data = in.readInt();
        }
        if (mode == MODE_QPSK) {
            sat_id = in.readInt();
            int satparams_notnull = in.readInt();
            if (satparams_notnull == 1)
                tv_satparams = new TvSatelliteParams(in);
            sat_polarisation = in.readInt();
        }
        if (mode == MODE_ISDBT) {
            isdbtLayer = in.readInt();
        }
    }

    public void writeToParcel(Parcel dest, int flags){
        dest.writeInt(mode);
        dest.writeInt(frequency);
        if ((mode == MODE_QAM) || (mode == MODE_QPSK))
            dest.writeInt(symbolRate);
        if ((mode == MODE_QAM) || (mode == MODE_ATSC))
            dest.writeInt(modulation);
        if (mode == MODE_OFDM || mode == MODE_DTMB) {
            dest.writeInt(bandwidth);
            dest.writeInt(ofdm_mode);
        }
        if (mode == MODE_ANALOG) {
            dest.writeInt(audio);
            dest.writeInt(standard);
            dest.writeInt(afc_data);
        }
        if (mode == MODE_QPSK) {
            dest.writeInt(sat_id);
            int satparams_notnull = 0;
            if (tv_satparams != null) {
                satparams_notnull = 1;
            } else {
                satparams_notnull = 0;
            }
            dest.writeInt(satparams_notnull);
            if (satparams_notnull == 1)
                tv_satparams.writeToParcel(dest, flags);
            dest.writeInt(sat_polarisation);
        }
        if (mode == MODE_ISDBT) {
            dest.writeInt(isdbtLayer);
        }
    }

    public TvChannelParams(Parcel in){
        readFromParcel(in);
    }

    public TvChannelParams(int mode){
        this.mode = mode;
    }


    public static TvChannelParams dvbcParams(int frequency, int modulation, int symbolRate){
        TvChannelParams tp = new TvChannelParams(MODE_QAM);

        tp.frequency  = frequency;
        tp.modulation = modulation;
        tp.symbolRate = symbolRate;

        return tp;
    }

    public static TvChannelParams dvbtParams(int frequency, int bandwidth){
        TvChannelParams tp = new TvChannelParams(MODE_OFDM);
        tp.frequency = frequency;
        tp.bandwidth = bandwidth;
        tp.ofdm_mode = OFDM_MODE_DVBT;

        return tp;
    }


    public static TvChannelParams dvbt2Params(int frequency, int bandwidth){
        TvChannelParams tp = new TvChannelParams(MODE_OFDM);
        Log.d(TAG,"---new DVBT2 channel params---");
        tp.frequency = frequency;
        tp.bandwidth = bandwidth;
        tp.ofdm_mode = OFDM_MODE_DVBT2;
        return tp;
    }


    /*	public static TvChannelParams dvbsParams(Context context, int frequency, int symbolRate, int sat_id, int sat_polarisation){
        TvChannelParams tp = new TvChannelParams(MODE_QPSK);

        tp.frequency  = frequency;
        tp.symbolRate = symbolRate;
        tp.sat_id = sat_id;
        tp.sat_polarisation = sat_polarisation;

        TVSatellite sat = TVSatellite.tvSatelliteSelect(context, sat_id);
        tp.tv_satparams = sat.getParams();

        return tp;
        }
     */

    public static TvChannelParams atscParams(int frequency, int modulation){
        TvChannelParams tp = new TvChannelParams(MODE_ATSC);

        tp.frequency  = frequency;
        tp.modulation = modulation;
        return tp;
    }


    public static TvChannelParams analogParams(int frequency, int std, int audio,int afc_flag){
        TvChannelParams tp = new TvChannelParams(MODE_ANALOG);

        tp.frequency = frequency;
        tp.audio     = audio;
        tp.standard  = std;
        tp.afc_data  = afc_flag;
        return tp;
    }

    public static TvChannelParams dtmbParams(int frequency, int bandwidth){
        TvChannelParams tp = new TvChannelParams(MODE_DTMB);

        tp.frequency = frequency;
        tp.bandwidth = bandwidth;

        return tp;
    }


    public static TvChannelParams isdbtParams(int frequency, int bandwidth){
        TvChannelParams tp = new TvChannelParams(MODE_ISDBT);

        tp.frequency = frequency;
        tp.bandwidth = bandwidth;
        tp.isdbtLayer = ISDBT_LAYER_ALL;

        return tp;
    }

    public static int AudioStd2Enum(int data){
        int std = TvControlManager.ATV_AUDIO_STD_DK;
        if (((data & STD_PAL_DK) == STD_PAL_DK) ||
                ((data & STD_SECAM_DK) == STD_SECAM_DK))
            std = TvControlManager.ATV_AUDIO_STD_DK;
        else if ((data & STD_PAL_I) == STD_PAL_I)
                return TvControlManager.ATV_AUDIO_STD_I;
        else if (((data & STD_PAL_BG) == STD_PAL_BG) ||
                ((data & STD_SECAM_B) == STD_SECAM_B)||
                ((data & STD_SECAM_G) == STD_SECAM_G ))
            std = TvControlManager.ATV_AUDIO_STD_BG;
        else if ( ((data & STD_PAL_M) == STD_PAL_M) ||
                ((data & STD_NTSC_M) == STD_NTSC_M))
            std = TvControlManager.ATV_AUDIO_STD_M;
        else if ( (data & STD_SECAM_L) == STD_SECAM_L)
            std = TvControlManager.ATV_AUDIO_STD_L;
        return std;
    }

    public static int VideoStd2Enum(int data) {
        int videostd = 0;
        if ((data & COLOR_PAL) == COLOR_PAL) {
            videostd = TvControlManager.ATV_VIDEO_STD_PAL;
        } else if ((data & COLOR_NTSC) == COLOR_NTSC) {
            videostd = TvControlManager.ATV_VIDEO_STD_NTSC;
        } else if ((data & COLOR_SECAM) == COLOR_SECAM) {
            videostd = TvControlManager.ATV_VIDEO_STD_SECAM;
        } else if ((data & COLOR_AUTO) == COLOR_AUTO) {
            videostd = TvControlManager.ATV_VIDEO_STD_AUTO;
        }

        return videostd;
    }

    public static int Change2VideoStd(int data){
        int videostd = 0;
        if (data == TvControlManager.ATV_VIDEO_STD_AUTO)
            videostd = COLOR_AUTO;
        else if (data == TvControlManager.ATV_VIDEO_STD_PAL)
            videostd = COLOR_PAL;
        else if (data == TvControlManager.ATV_VIDEO_STD_NTSC)
            videostd = COLOR_NTSC;
        else if (data == TvControlManager.ATV_VIDEO_STD_SECAM)
            videostd = COLOR_SECAM;
        return videostd;
    }

    public static int Change2AudioStd(int video_std,int audio_std) {
        int tmpTunerStd = 0;

        if (audio_std >= 0) {
            if (audio_std < TvControlManager.ATV_AUDIO_STD_DK
                    || audio_std > TvControlManager.ATV_AUDIO_STD_AUTO) {
                //audio_std = ATVHandleAudioStdCfg();
            }
        }
        if (video_std == TvControlManager.ATV_VIDEO_STD_AUTO) {
            //tmpTunerStd |= CC_ATV_ADUIO_STANDARD.TUNER_COLOR_PAL;
            if (audio_std == TvControlManager.ATV_AUDIO_STD_DK) {
                tmpTunerStd |= STD_PAL_DK;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_I) {
                tmpTunerStd |= STD_PAL_I;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_BG) {
                tmpTunerStd |= STD_PAL_BG;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_M) {
                tmpTunerStd |= STD_NTSC_M;
            }
        } else if (video_std == TvControlManager.ATV_VIDEO_STD_PAL) {
            // tmpTunerStd |= CC_ATV_ADUIO_STANDARDCOLOR_PAL;
            if (audio_std == TvControlManager.ATV_AUDIO_STD_DK) {
                tmpTunerStd |= STD_PAL_DK;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_I) {
                tmpTunerStd |= STD_PAL_I;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_BG) {
                tmpTunerStd |= STD_PAL_BG;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_M) {
                tmpTunerStd |= STD_PAL_M;
            }
        } else if (video_std == TvControlManager.ATV_VIDEO_STD_NTSC) {
            // tmpTunerStd |= CC_ATV_ADUIO_STANDARD.COLOR_NTSC;
            if (audio_std == TvControlManager.ATV_AUDIO_STD_DK) {
                tmpTunerStd |= STD_PAL_DK;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_I) {
                tmpTunerStd |= STD_PAL_I;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_BG) {
                tmpTunerStd |= STD_PAL_BG;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_M) {
                tmpTunerStd |= STD_NTSC_M;
            }
        } else if (video_std == TvControlManager.ATV_VIDEO_STD_SECAM) {
            // tmpTunerStd |= TUNER_COLOR_SECAM;
            if (audio_std == TvControlManager.ATV_AUDIO_STD_DK) {
                tmpTunerStd |= STD_SECAM_DK;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_I) {
                tmpTunerStd |= STD_PAL_I;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_BG) {
                tmpTunerStd |= (STD_SECAM_B | STD_SECAM_G);
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_M) {
                tmpTunerStd |= STD_NTSC_M;
            } else if (audio_std == TvControlManager.ATV_AUDIO_STD_L) {
                tmpTunerStd |= STD_SECAM_L;
            }
        }
        return tmpTunerStd;
    }

    public static int getTunerStd(int video_std,int audio_std){
        int vdata = Change2VideoStd(video_std);
        int adata = Change2AudioStd(video_std, audio_std);
        return (vdata|adata);
    }

    static int ATVHandleAudioStdCfg(String key) {
        if (key.equals( "DK") ) {
            return TvControlManager.ATV_AUDIO_STD_DK;
        } else if (key.equals( "I")) {
            return TvControlManager.ATV_AUDIO_STD_I;
        } else if (key.equals( "BG") ) {
            return TvControlManager.ATV_AUDIO_STD_BG;
        } else if (key.equals( "M") ) {
            return TvControlManager.ATV_AUDIO_STD_M;
        } else if ( key.equals("L") ) {
            return TvControlManager.ATV_AUDIO_STD_L;
        } else if (key.equals( "AUTO") ) {
            return TvControlManager.ATV_AUDIO_STD_AUTO;
        }
        return TvControlManager.ATV_AUDIO_STD_AUTO;
    }

    public boolean setATVAudio(int audio){
        if (this.audio == audio)
            return false;

        this.audio = audio;
        return true;
    }

    public boolean setATVVideoFormat(int fmt){
        int afmt = AudioStd2Enum(standard);
        int std = getTunerStd(fmt, afmt);

        if (std == standard)
            return false;

        standard = std;
        return true;
    }

    public boolean setATVAudioFormat(int fmt){
        int vfmt = VideoStd2Enum(standard);
        int std = getTunerStd(vfmt, fmt);

        if (std == standard)
            return false;

        standard = std;
        return true;
    }


    public int getMode(){
        return mode;
    }


    public boolean isDVBMode(){
        if ((mode == MODE_QPSK) || (mode == MODE_QAM) ||
                (mode == MODE_OFDM) || (mode == MODE_DTMB)) {
            return true;
        }

        return false;
    }

    public boolean isDVBCMode(){
        if (mode == MODE_QAM) {
            return true;
        }

        return false;
    }


    public boolean isDVBTMode(){
        if (mode == MODE_OFDM) {
            return true;
        }

        return false;
    }


    public boolean isDVBSMode(){
        if (mode == MODE_QPSK) {
            return true;
        }

        return false;
    }


    public boolean isATSCMode(){
        if (mode == MODE_ATSC) {
            return true;
        }

        return false;
    }

    public boolean isAnalogMode(){
        if (mode == MODE_ANALOG) {
            return true;
        }

        return false;
    }


    public boolean isDTMBMode(){
        if (mode == MODE_DTMB) {
            return true;
        }

        return false;
    }


    public boolean isISDBTMode(){
        if (mode == MODE_ISDBT) {
            return true;
        }

        return false;
    }


    public int getFrequency(){
        return frequency;
    }


    public int getOFDM_Mode(){
        return ofdm_mode;
    }


    public void setFrequency(int frequency){
        this.frequency = frequency;
    }


    public int getAudioMode(){
        if (!isAnalogMode())
            throw new UnsupportedOperationException();

        return audio;
    }


    public int getStandard(){
        if (!isAnalogMode())
            throw new UnsupportedOperationException();

        return standard;
    }


    public int getBandwidth(){
        if (mode != MODE_OFDM && mode != MODE_DTMB)
            throw new UnsupportedOperationException();

        return bandwidth;
    }

    public int getModulation(){
        if (mode != MODE_QAM)
            throw new UnsupportedOperationException();

        return modulation;
    }


    public int getSymbolRate(){
        if (!((mode == MODE_QPSK) || (mode == MODE_QAM)))
            throw new UnsupportedOperationException();

        return symbolRate;
    }


    public void setSymbolRate(int symbolRate){
        if (!((mode == MODE_QPSK) || (mode == MODE_QAM)))
            throw new UnsupportedOperationException();

        this.symbolRate = symbolRate;
    }


    public int getSatId(){
        if (mode != MODE_QPSK)
            throw new UnsupportedOperationException();

        return sat_id;
    }


    public int getPolarisation(){
        if (mode != MODE_QPSK)
            throw new UnsupportedOperationException();

        return this.sat_polarisation;
    }


    public int getISDBTLayer(){
        if (mode != MODE_ISDBT)
            throw new UnsupportedOperationException();

        return isdbtLayer;
    }


    public void setISDBTLayer(int layer){
        if (mode != MODE_ISDBT)
            throw new UnsupportedOperationException();

        this.isdbtLayer = isdbtLayer;
    }


    public void setPolarisation(int sat_polarisation){
        if (mode != MODE_QPSK)
            throw new UnsupportedOperationException();

        this.sat_polarisation = sat_polarisation;
    }


    public boolean equals(TvChannelParams params){
        if (this.mode != params.mode)
            return false;

        if (this.mode == MODE_ANALOG &&
                Math.abs(this.frequency - params.frequency) > 2000000)
            return false;

        if (this.frequency != params.frequency)
            return false;

        if (this.mode == MODE_QPSK) {
            if (this.sat_polarisation != params.sat_polarisation)
                return false;

            if (!(this.tv_satparams.equals(params.tv_satparams)))
                return false;
        }

        return true;
    }

    public boolean equals_frontendevt(TvChannelParams params){
        if (this.mode != params.mode)
            return false;

        if (this.mode == MODE_QPSK) {
            /*freq calc*/
        } else {
            if (this.frequency != params.frequency)
                return false;
        }

        return true;
    }

    public int describeContents(){
        return 0;
    }

    public static Parcelable.Creator<TvChannelParams> getCreator() {
        return CREATOR;
    }
}

