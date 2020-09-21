/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.car.procfsinspector;

import android.os.Parcel;
import android.os.Parcelable;
import java.util.Objects;

public class ProcessInfo implements Parcelable {
    public static final Parcelable.Creator<ProcessInfo> CREATOR =
        new Parcelable.Creator<ProcessInfo>() {
            public ProcessInfo createFromParcel(Parcel in) {
                return new ProcessInfo(in);
            }

            public ProcessInfo[] newArray(int size) {
                return new ProcessInfo[size];
            }
        };

    public final int pid;
    public final int uid;

    public ProcessInfo(int pid, int uid) {
        this.pid = pid;
        this.uid = uid;
    }

    public ProcessInfo(Parcel in) {
        this.pid = in.readInt();
        this.uid = in.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(pid);
        dest.writeInt(uid);
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof ProcessInfo) {
            ProcessInfo processInfo = (ProcessInfo)other;
            return processInfo.pid == pid && processInfo.uid == uid;
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(pid, uid);
    }

    @Override
    public String toString() {
        return String.format("pid = %d, uid = %d", pid, uid);
    }
}
