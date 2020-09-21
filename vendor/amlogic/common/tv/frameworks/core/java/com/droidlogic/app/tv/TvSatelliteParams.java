/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.os.Parcel;
import android.os.Parcelable;
import android.database.Cursor;
import android.content.Context;
import android.util.Log;


public class TvSatelliteParams implements Parcelable {
    private static final String TAG="TvSatelliteParams";

    public static final int SEC_22k_ON = 0;
    public static final int SEC_22k_OFF = 1;
    public static final int SEC_22k_AUTO = 2;

    public static final int SEC_VOLTAGE_13V = 0;
    public static final int SEC_VOLTAGE_18V = 1;
    public static final int SEC_VOLTAGE_OFF = 2;
    public static final int SEC_VOLTAGE_AUTO = 3;

    public static final int SEC_TONE_BURST_NONE = 0;
    public static final int SEC_TONE_BURST_A = 1;
    public static final int SEC_TONE_BURST_B = 2;

    public static final int DISEQC_MODE_NONE = 0;
    public static final int DISEQC_MODE_V1_0 = 1;
    public static final int DISEQC_MODE_V1_1 = 2;
    public static final int DISEQC_MODE_V1_2 = 3;
    public static final int DISEQC_MODE_V1_3 = 4;
    public static final int DISEQC_MODE_SMARTV = 5;

    public static final int DISEQC_COMMITTED_AA = 0;
    public static final int DISEQC_COMMITTED_AB = 1;
    public static final int DISEQC_COMMITTED_BA = 2;
    public static final int DISEQC_COMMITTED_BB = 3;

    public static final int DISEQC_NONE  = 4;

    public static final int DISEQC_UNCOMMITTED_0 = 0xF0;
    public static final int DISEQC_UNCOMMITTED_1 = 0xF1;
    public static final int DISEQC_UNCOMMITTED_2 = 0xF2;
    public static final int DISEQC_UNCOMMITTED_3 = 0xF3;
    public static final int DISEQC_UNCOMMITTED_4 = 0xF4;
    public static final int DISEQC_UNCOMMITTED_5 = 0xF5;
    public static final int DISEQC_UNCOMMITTED_6 = 0xF6;
    public static final int DISEQC_UNCOMMITTED_7 = 0xF7;
    public static final int DISEQC_UNCOMMITTED_8 = 0xF8;
    public static final int DISEQC_UNCOMMITTED_9 = 0xF9;
    public static final int DISEQC_UNCOMMITTED_10 = 0xFA;
    public static final int DISEQC_UNCOMMITTED_11 = 0xFB;
    public static final int DISEQC_UNCOMMITTED_12 = 0xFC;
    public static final int DISEQC_UNCOMMITTED_13 = 0xFD;
    public static final int DISEQC_UNCOMMITTED_14 = 0xFE;
    public static final int DISEQC_UNCOMMITTED_15 = 0xFF;

    public double local_longitude;
    public double local_latitude;

    public int lnb_num;
    public int lnb_lof_hi;
    public int lnb_lof_lo;
    public int lnb_lof_threadhold;
    public int sec_22k_status;
    public int sec_voltage_status;
    public int sec_tone_burst;
    public int diseqc_mode;
    public int diseqc_committed;
    public int diseqc_uncommitted;
    public int diseqc_repeat_count;
    public int diseqc_sequence_repeat;
    public int diseqc_fast;
    /* 	diseqc 1.0)
        0) commited, toneburst
        1) toneburst, committed
        diseqc > 1.0)
        2) committed, uncommitted, toneburst
        3) toneburst, committed, uncommitted
        4) uncommitted, committed, toneburst
        5) toneburst, uncommitted, committed */
    public int diseqc_order;
    public int motor_num;
    /*1-255, 0 ref positon*/
    public int motor_position_num;
    public double sat_longitude;

    /** Dtv-Sx set Unicable settings, only use for dev_set_frontend*/
    private int user_band;
    private int ub_freq;//!< kHz

    public static final Parcelable.Creator<TvSatelliteParams> CREATOR = new Parcelable.Creator<TvSatelliteParams>(){
        public TvSatelliteParams createFromParcel(Parcel in) {
            return new TvSatelliteParams(in);
        }
        public TvSatelliteParams[] newArray(int size) {
            return new TvSatelliteParams[size];
        }
    };

    public void readFromParcel(Parcel in){
        local_longitude = in.readDouble();
        local_latitude = in.readDouble();
        lnb_num = in.readInt();
        lnb_lof_hi = in.readInt();
        lnb_lof_lo = in.readInt();
        lnb_lof_threadhold = in.readInt();
        sec_22k_status = in.readInt();
        sec_voltage_status = in.readInt();
        sec_tone_burst = in.readInt();
        diseqc_mode = in.readInt();
        diseqc_committed = in.readInt();
        diseqc_uncommitted = in.readInt();
        diseqc_repeat_count = in.readInt();
        diseqc_sequence_repeat = in.readInt();
        diseqc_fast = in.readInt();
        diseqc_order = in.readInt();
        motor_num = in.readInt();
        motor_position_num = in.readInt();
        sat_longitude = in.readDouble();
        user_band = in.readInt();
        ub_freq = in.readInt();
    }

    public void writeToParcel(Parcel dest, int flags){
        dest.writeDouble(local_longitude);
        dest.writeDouble(local_latitude);
        dest.writeInt(lnb_num);
        dest.writeInt(lnb_lof_hi);
        dest.writeInt(lnb_lof_lo);
        dest.writeInt(lnb_lof_threadhold);
        dest.writeInt(sec_22k_status);
        dest.writeInt(sec_voltage_status);
        dest.writeInt(sec_tone_burst);
        dest.writeInt(diseqc_mode);
        dest.writeInt(diseqc_committed);
        dest.writeInt(diseqc_uncommitted);
        dest.writeInt(diseqc_repeat_count);
        dest.writeInt(diseqc_sequence_repeat);
        dest.writeInt(diseqc_fast);
        dest.writeInt(diseqc_order);
        dest.writeInt(motor_num);
        dest.writeInt(motor_position_num);
        dest.writeDouble(sat_longitude);
        dest.writeInt(user_band);
        dest.writeInt(ub_freq);
    }

    public TvSatelliteParams(Parcel in){
        readFromParcel(in);
    }

    public TvSatelliteParams(){
    }

    public TvSatelliteParams(double sat_longitude){
        /*default value*/
        this.sat_longitude = sat_longitude;
    }


    public void setSatelliteRecLocal(double local_longitude, double local_latitude){
        this.local_longitude = local_longitude;
        this.local_latitude = local_latitude;
    }


    public double getSatelliteRecLocalLongitude(){
        return this.local_longitude;
    }


    public double getSatelliteRecLocalLatitude(){
        return this.local_latitude;
    }


    public void setSatelliteLnb(int lnb_num, int lnb_lof_hi, int lnb_lof_lo, int lnb_lof_threadhold){
        this.lnb_num = lnb_num;
        this.lnb_lof_hi = lnb_lof_hi;
        this.lnb_lof_lo = lnb_lof_lo;
        this.lnb_lof_threadhold = lnb_lof_threadhold;
    }


    public int getSatelliteLnbNum(){
        return this.lnb_num;
    }


    public int getSatelliteLnbLofhi(){
        return this.lnb_lof_hi;
    }


    public int getSatelliteLnbLofLo(){
        return this.lnb_lof_lo;
    }

    public int getSatelliteLnbLofthreadhold(){
        return this.lnb_lof_threadhold;
    }

    public void setSec22k(int sec_22k_status){
        this.sec_22k_status = sec_22k_status;
    }


    public int getSec22k(){
        return this.sec_22k_status;
    }


    public void setSecVoltage(int sec_voltage_status){
        this.sec_voltage_status = sec_voltage_status;
    }


    public int getSecVoltage(){
        return this.sec_voltage_status;
    }

    public void setSecToneBurst(int sec_tone_burst){
        this.sec_tone_burst = sec_tone_burst;
    }


    public int getSecToneBurst(){
        return this.sec_tone_burst;
    }

    public void setDiseqcMode(int diseqc_mode){
        this.diseqc_mode = diseqc_mode;
    }


    public int getDiseqcMode(){
        return this.diseqc_mode;
    }

    public void setDiseqcCommitted(int diseqc_committed){
        this.diseqc_committed = diseqc_committed;
    }


    public int getDiseqcCommitted(){
        return this.diseqc_committed;
    }

    public void setDiseqcUncommitted(int diseqc_uncommitted){
        this.diseqc_uncommitted= diseqc_uncommitted;
    }


    public int getDiseqcUncommitted(){
        return this.diseqc_uncommitted;
    }

    public void setDiseqcRepeatCount(int diseqc_repeat_count){
        this.diseqc_repeat_count = diseqc_repeat_count;
    }


    public int getDiseqcRepeatCount(){
        return this.diseqc_repeat_count;
    }


    public void setDiseqcSequenceRepeat(int diseqc_sequence_repeat){
        this.diseqc_sequence_repeat = diseqc_sequence_repeat;
    }


    public int getDiseqcSequenceRepeat(){
        return this.diseqc_sequence_repeat;
    }


    public void setDiseqcFast(int diseqc_fast){
        this.diseqc_fast = diseqc_fast;
    }


    public int getDiseqcFast(){
        return this.diseqc_fast;
    }

    public void setDiseqcOrder(int diseqc_order){
        this.diseqc_order = diseqc_order;
    }


    public int getDiseqcOrder(){
        return this.diseqc_order;
    }


    public void setMotorNum(int motor_num){
        this.motor_num = motor_num;
    }


    public int getMotorNum(){
        return this.motor_num;
    }


    public void setMotorPositionNum(int motor_position_num){
        this.motor_position_num = motor_position_num;
    }


    public int getMotorPositionNum(){
        return this.motor_position_num;
    }


    public void setSatelliteLongitude(double sat_longitude){
        this.sat_longitude = sat_longitude;
    }


    public double getSatelliteLongitude(){
        return this.sat_longitude;
    }


    public void setUnicableParams(int user_band, int ub_freq) {
        this.user_band = user_band;
        this.ub_freq = ub_freq;
    }


    public int getUnicableUserband(){
        return this.user_band;
    }


    public int getUnicableUbfreq(){
        return this.ub_freq;
    }


    public boolean equals(TvSatelliteParams params){
        if (this.local_longitude != params.local_longitude)
            return false;

        if (this.local_latitude != params.local_latitude)
            return false;

        if (this.lnb_num != params.lnb_num)
            return false;

        if (this.lnb_lof_hi != params.lnb_lof_hi)
            return false;

        if (this.lnb_lof_lo != params.lnb_lof_lo)
            return false;

        if (this.lnb_lof_threadhold != params.lnb_lof_threadhold)
            return false;

        if (this.sec_22k_status != params.sec_22k_status)
            return false;

        if (this.sec_voltage_status != params.sec_voltage_status)
            return false;

        if (this.sec_tone_burst != params.sec_tone_burst)
            return false;

        if (this.diseqc_mode != params.diseqc_mode)
            return false;

        if (this.diseqc_committed != params.diseqc_committed)
            return false;

        if (this.diseqc_uncommitted != params.diseqc_uncommitted)
            return false;

        if (this.diseqc_repeat_count != params.diseqc_repeat_count)
            return false;

        if (this.diseqc_sequence_repeat != params.diseqc_sequence_repeat)
            return false;

        if (this.diseqc_fast != params.diseqc_fast)
            return false;

        if (this.diseqc_order != params.diseqc_order)
            return false;

        if (this.motor_num != params.motor_num)
            return false;

        if (this.motor_position_num != params.motor_position_num)
            return false;

        if (this.sat_longitude != params.sat_longitude)
            return false;

        if (this.user_band != params.user_band)
            return false;

        if (this.ub_freq != params.ub_freq)
            return false;

        return true;
    }

    public int describeContents(){
        return 0;
    }

    public static Parcelable.Creator<TvSatelliteParams> getCreator() {
        return CREATOR;
    }
}

