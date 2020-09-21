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

/**
 * Created by yu.fang on 2018/7/3.
 */

public class EventParams implements Parcelable{
    private int frequency;
    private int quality;
    private int strength;
    private int channelNumber;
    private int radioNumber;

    public EventParams() {
    }

    public EventParams(int frequency, int quality, int strength, int channelNumber, int radioNumber) {
        this.frequency = frequency;
        this.quality = quality;
        this.strength = strength;
        this.channelNumber = channelNumber;
        this.radioNumber = radioNumber;
    }

    protected EventParams(Parcel in) {
        frequency = in.readInt();
        quality = in.readInt();
        strength = in.readInt();
        channelNumber = in.readInt();
        radioNumber = in.readInt();
    }

    public int getFrequency() {
        return frequency;
    }

    public int getQuality() {
        return quality;
    }

    public int getStrength() {
        return strength;
    }

    public int getChannelNumber() {
        return channelNumber;
    }

    public int getRadioNumber() {
        return radioNumber;
    }

    public static Creator<EventParams> getCREATOR() {
        return CREATOR;
    }

    public static final Creator<EventParams> CREATOR = new Creator<EventParams>() {
        @Override
        public EventParams createFromParcel(Parcel in) {
            return new EventParams(in);
        }

        @Override
        public EventParams[] newArray(int size) {
            return new EventParams[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(frequency);
        dest.writeInt(quality);
        dest.writeInt(strength);
        dest.writeInt(channelNumber);
        dest.writeInt(radioNumber);
    }
}
