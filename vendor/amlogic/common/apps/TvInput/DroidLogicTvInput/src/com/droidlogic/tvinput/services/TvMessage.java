/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by yu.fang on 2018/7/3.
 */

public class TvMessage implements Parcelable{
    private int type;
    private int message;
    private String information;

    public TvMessage() {}

    public TvMessage(int type, int message, String information) {
        this.type = type;
        this.message = message;
        this.information = information;
    }

    protected TvMessage(Parcel in) {
        type = in.readInt();
        message = in.readInt();
        information = in.readString();
    }

    public int getType() {
        return type;
    }

    public int getMessage() {
        return message;
    }

    public String getInformation() {
        return information;
    }

    public static Creator<TvMessage> getCREATOR() {
        return CREATOR;
    }

    public static final Creator<TvMessage> CREATOR = new Creator<TvMessage>() {
        @Override
        public TvMessage createFromParcel(Parcel in) {
            return new TvMessage(in);
        }

        @Override
        public TvMessage[] newArray(int size) {
            return new TvMessage[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(type);
        dest.writeInt(message);
        dest.writeString(information);
    }
}
