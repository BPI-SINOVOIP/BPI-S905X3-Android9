/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.car.drivingstate;

import android.annotation.IntDef;
import android.annotation.SystemApi;
import android.os.Parcel;
import android.os.Parcelable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Driving State related events.  Driving State of a car conveys if the car is currently parked,
 * idling or moving.
 *
 * @hide
 */
@SystemApi
public class CarDrivingStateEvent implements Parcelable {

    // New Driving States
    /**
     * This is when we don't have enough information to infer the car's driving state.
     */
    public static final int DRIVING_STATE_UNKNOWN = -1;
    /**
     * Car is parked - Gear is in Parked mode.
     */
    public static final int DRIVING_STATE_PARKED = 0;
    /**
     * Car is idling.  Gear is not in Parked mode and Speed of the vehicle is zero.
     * TODO: (b/72157869) - Should speed that differentiates moving vs idling be configurable?
     */
    public static final int DRIVING_STATE_IDLING = 1;
    /**
     * Car is moving.  Gear is not in parked mode and speed of the vehicle is non zero.
     */
    public static final int DRIVING_STATE_MOVING = 2;

    /** @hide */
    @IntDef({DRIVING_STATE_UNKNOWN,
            DRIVING_STATE_PARKED,
            DRIVING_STATE_IDLING,
            DRIVING_STATE_MOVING})
    @Retention(RetentionPolicy.SOURCE)
    public @interface CarDrivingState {
    }

    /**
     * Time at which this driving state was inferred based on the car's sensors.
     * It is the elapsed time in nanoseconds since system boot.
     */
    public final long timeStamp;

    /**
     * The Car's driving state.
     */
    @CarDrivingState
    public final int eventValue;


    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(eventValue);
        dest.writeLong(timeStamp);
    }

    public static final Parcelable.Creator<CarDrivingStateEvent> CREATOR
            = new Parcelable.Creator<CarDrivingStateEvent>() {
        public CarDrivingStateEvent createFromParcel(Parcel in) {
            return new CarDrivingStateEvent(in);
        }

        public CarDrivingStateEvent[] newArray(int size) {
            return new CarDrivingStateEvent[size];
        }
    };

    public CarDrivingStateEvent(int value, long time) {
        eventValue = value;
        timeStamp = time;
    }

    private CarDrivingStateEvent(Parcel in) {
        eventValue = in.readInt();
        timeStamp = in.readLong();
    }

    @Override
    public String toString() {
        return eventValue + " " + timeStamp;
    }
}
