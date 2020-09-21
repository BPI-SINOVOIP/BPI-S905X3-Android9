/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.car.hardware.cabin;

import android.annotation.IntDef;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyManager;
import android.content.Context;
import android.os.Handler;
import android.os.IBinder;
import android.util.ArraySet;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

/**
 * API for controlling Cabin system in cars.
 * Most Car Cabin properties have both a MOVE and POSITION parameter associated with them.
 *
 * The MOVE parameter will start moving the device in the indicated direction.  Magnitude
 * indicates relative speed.  For instance, setting the WINDOW_MOVE parameter to +1 rolls
 * the window down.  Setting it to +2 (if available) will roll it down faster.
 *
 * POSITION parameter will move the device to the desired position.  For instance, if the
 * WINDOW_POS has a range of 0-100, setting this parameter to 50 will open the window
 * halfway.  Depending upon the initial position, the window may move up or down to the
 * 50% value.
 *
 * One or both of the MOVE/POSITION parameters may be implemented depending upon the
 * capability of the hardware.
 * @hide
 */
@SystemApi
public final class CarCabinManager implements CarManagerBase {
    private final static boolean DBG = false;
    private final static String TAG = "CarCabinManager";
    private final CarPropertyManager mCarPropertyMgr;
    private final ArraySet<CarCabinEventCallback> mCallbacks = new ArraySet<>();
    private CarPropertyEventListenerToBase mListenerToBase = null;

    /** Door properties are zoned by VehicleAreaDoor */
    /**
     * door position, int type
     * Max value indicates fully open, min value (0) indicates fully closed.
     *
     * Some vehicles (minivans) can open the door electronically.  Hence, the ability
     * to write this property.
     */
    public static final int ID_DOOR_POS = 0x16400b00;
    /** door move, int type
     * Positive values open the door, negative values close it.
     */
    public static final int ID_DOOR_MOVE = 0x16400b01;
    /** door lock, bool type
     * 'true' indicates door is locked.
     */
    public static final int ID_DOOR_LOCK = 0x16200b02;

    /** Mirror properties are zoned by VehicleAreaMirror */
    /**
     * mirror z position, int type
     * Positive value indicates tilt upwards, negative value tilt downwards.
     */
    public static final int ID_MIRROR_Z_POS = 0x14400b40;
    /** mirror z move, int type
     * Positive value tilts the mirror upwards, negative value tilts downwards.
     */
    public static final int ID_MIRROR_Z_MOVE = 0x14400b41;
    /**
     * mirror y position, int type
     * Positive value indicates tilt right, negative value tilt left
     */
    public static final int ID_MIRROR_Y_POS = 0x14400b42;
    /** mirror y move, int type
     * Positive value tilts the mirror right, negative value tilts left.
     */
    public static final int ID_MIRROR_Y_MOVE = 0x14400b43;
    /**
     * mirror lock, bool type
     * True indicates mirror positions are locked and not changeable.
     */
    public static final int ID_MIRROR_LOCK = 0x11200b44;
    /**
     * mirror fold, bool type
     * True indicates mirrors are folded.
     */
    public static final int ID_MIRROR_FOLD = 0x11200b45;

    /** Seat properties are zoned by VehicleAreaSeat */
    /**
     * seat memory select, int type
     * This parameter selects the memory preset to use to select the seat position.
     * The minValue is always 1, and the maxValue determines the number of seat
     * positions available.
     *
     * For instance, if the driver's seat has 3 memory presets, the maxValue will be 3.
     * When the user wants to select a preset, the desired preset number (1, 2, or 3)
     * is set.
     */
    public static final int ID_SEAT_MEMORY_SELECT = 0x15400b80;
    /**
     * seat memory set, int type
     * This setting allows the user to save the current seat position settings into
     * the selected preset slot.  The maxValue for each seat position shall match
     * the maxValue for VEHICLE_PROPERTY_SEAT_MEMORY_SELECT.
     */
    public static final int ID_SEAT_MEMORY_SET = 0x15400b81;
    /**
     * seat belt buckled, bool type
     * True indicates belt is buckled.
     */
    public static final int ID_SEAT_BELT_BUCKLED = 0x15200b82;
    /**
     * seat belt height position, int type
     * Adjusts the shoulder belt anchor point.
     * Max value indicates highest position.
     * Min value indicates lowest position.
     */
    public static final int ID_SEAT_BELT_HEIGHT_POS = 0x15400b83;
    /** seat belt height move, int type
     * Adjusts the shoulder belt anchor point.
     * Positive value moves towards highest point.
     * Negative value moves towards lowest point.
     */
    public static final int ID_SEAT_BELT_HEIGHT_MOVE = 0x15400b84;
    /**
     * seat fore/aft position, int type
     * Sets the seat position forward (closer to steering wheel) and backwards.
     * Max value indicates closest to wheel, min value indicates most rearward position.
     */
    public static final int ID_SEAT_FORE_AFT_POS = 0x15400b85;
    /**
     * seat fore/aft move, int type
     * Positive value moves seat forward (closer to steering wheel).
     * Negative value moves seat rearward.
     */
    public static final int ID_SEAT_FORE_AFT_MOVE = 0x15400b86;
    /**
     * seat backrest angle #1 position, int type
     * Backrest angle 1 is the actuator closest to the bottom of the seat.
     * Max value indicates angling forward towards the steering wheel.
     * Min value indicates full recline.
     */
    public static final int ID_SEAT_BACKREST_ANGLE_1_POS = 0x15400b87;
    /** seat backrest angle #1 move, int type
     * Backrest angle 1 is the actuator closest to the bottom of the seat.
     * Positive value angles seat towards the steering wheel.
     * Negatie value angles away from steering wheel.
     */
    public static final int ID_SEAT_BACKREST_ANGLE_1_MOVE = 0x15400b88;
    /**
     * seat backrest angle #2 position, int type
     * Backrest angle 2 is the next actuator up from the bottom of the seat.
     * Max value indicates angling forward towards the steering wheel.
     * Min value indicates full recline.
     */
    public static final int ID_SEAT_BACKREST_ANGLE_2_POS = 0x15400b89;
    /** seat backrest angle #2 move, int type
     * Backrest angle 2 is the next actuator up from the bottom of the seat.
     * Positive value tilts forward towards the steering wheel.
     * Negative value tilts backwards.
     */
    public static final int ID_SEAT_BACKREST_ANGLE_2_MOVE = 0x15400b8a;
    /**
     * seat height position, int type
     * Sets the seat height.
     * Max value indicates highest position.
     * Min value indicates lowest position.
     */
    public static final int ID_SEAT_HEIGHT_POS = 0x15400b8b;
    /** seat height move, int type
     * Sets the seat height.
     * Positive value raises the seat.
     * Negative value lowers the seat.
     * */
    public static final int ID_SEAT_HEIGHT_MOVE = 0x15400b8c;
    /**
     * seat depth position, int type
     * Sets the seat depth, distance from back rest to front edge of seat.
     * Max value indicates longest depth position.
     * Min value indicates shortest position.
     */
    public static final int ID_SEAT_DEPTH_POS = 0x15400b8d;
    /** seat depth move, int type
     * Adjusts the seat depth, distance from back rest to front edge of seat.
     * Positive value increases the distance from back rest to front edge of seat.
     * Negative value decreases this distance.
     */
    public static final int ID_SEAT_DEPTH_MOVE = 0x15400b8e;
    /**
     * seat tilt position, int type
     * Sets the seat tilt.
     * Max value indicates front edge of seat higher than back edge.
     * Min value indicates front edge of seat lower than back edge.
     */
    public static final int ID_SEAT_TILT_POS = 0x15400b8f;
    /** seat tilt move, int type
     * Adjusts the seat tilt.
     * Positive value lifts front edge of seat higher than back edge.
     * Negative value lowers front edge of seat in relation to back edge.
     */
    public static final int ID_SEAT_TILT_MOVE = 0x15400b90;
    /**
     * seat lumbar fore/aft position, int type
     * Pushes the lumbar support forward and backwards.
     * Max value indicates most forward position.
     * Min value indicates most rearward position.
     */
    public static final int ID_SEAT_LUMBAR_FORE_AFT_POS = 0x15400b91;
    /** seat lumbar fore/aft move, int type
     * Adjusts the lumbar support forwards and backwards.
     * Positive value moves lumbar support forward.
     * Negative value moves lumbar support rearward.
     */
    public static final int ID_SEAT_LUMBAR_FORE_AFT_MOVE = 0x15400b92;
    /**
     * seat lumbar side support position, int type
     * Sets the amount of lateral lumbar support.
     * Max value indicates widest lumbar setting (i.e. least support)
     * Min value indicates thinnest lumbar setting.
     */
    public static final int ID_SEAT_LUMBAR_SIDE_SUPPORT_POS = 0x15400b93;
    /** seat lumbar side support move, int type
     * Adjusts the amount of lateral lumbar support.
     * Positive value widens the lumbar area.
     * Negative value makes the lumbar area thinner.
     */
    public static final int ID_SEAT_LUMBAR_SIDE_SUPPORT_MOVE = 0x15400b94;
    /**
     * seat headrest height position, int type
     * Sets the headrest height.
     * Max value indicates tallest setting.
     * Min value indicates shortest setting.
     */
    public static final int ID_SEAT_HEADREST_HEIGHT_POS = 0x15400b95;
    /** seat headrest height move, int type
     * Postive value moves the headrest higher.
     * Negative value moves the headrest lower.
     */
    public static final int ID_SEAT_HEADREST_HEIGHT_MOVE = 0x15400b96;
    /**
     * seat headrest angle position, int type
     * Sets the angle of the headrest.
     * Max value indicates most upright angle.
     * Min value indicates shallowest headrest angle.
     */
    public static final int ID_SEAT_HEADREST_ANGLE_POS = 0x15400b97;
    /** seat headrest angle move, int type
     * Adjusts the angle of the headrest.
     * Positive value angles headrest towards most upright angle.
     * Negative value angles headrest towards shallowest headrest angle.
     */
    public static final int ID_SEAT_HEADREST_ANGLE_MOVE = 0x15400b98;
    /**
     * seat headrest fore/aft position, int type
     * Sets the headrest forwards and backwards.
     * Max value indicates position closest to front of car.
     * Min value indicates position closest to rear of car.
     */
    public static final int ID_SEAT_HEADREST_FORE_AFT_POS = 0x15400b99;
    /** seat headrest fore/aft move, int type
     * Adjsuts the headrest forwards and backwards.
     * Positive value moves the headrest closer to front of car.
     * Negative value moves the headrest closer to rear of car.
     */
    public static final int ID_SEAT_HEADREST_FORE_AFT_MOVE = 0x15400b9a;

    /** Window properties are zoned by VehicleAreaWindow */
    /**
     * window position, int type
     * Max = window down / open.
     * Min = window up / closed.
     */
    public static final int ID_WINDOW_POS = 0x13400bc0;
    /** window move, int type
     * Positive value moves window down / opens window.
     * Negative value moves window up / closes window.
     */
    public static final int ID_WINDOW_MOVE = 0x13400bc1;
    /**
     * window lock, bool type
     * True indicates windows are locked and can't be moved.
     */
    public static final int ID_WINDOW_LOCK = 0x13400bc4;

    /** @hide */
    @IntDef({
            ID_DOOR_POS,
            ID_DOOR_MOVE,
            ID_DOOR_LOCK,
            ID_MIRROR_Z_POS,
            ID_MIRROR_Z_MOVE,
            ID_MIRROR_Y_POS,
            ID_MIRROR_Y_MOVE,
            ID_MIRROR_LOCK,
            ID_MIRROR_FOLD,
            ID_SEAT_MEMORY_SELECT,
            ID_SEAT_MEMORY_SET,
            ID_SEAT_BELT_BUCKLED,
            ID_SEAT_BELT_HEIGHT_POS,
            ID_SEAT_BELT_HEIGHT_MOVE,
            ID_SEAT_FORE_AFT_POS,
            ID_SEAT_FORE_AFT_MOVE,
            ID_SEAT_BACKREST_ANGLE_1_POS,
            ID_SEAT_BACKREST_ANGLE_1_MOVE,
            ID_SEAT_BACKREST_ANGLE_2_POS,
            ID_SEAT_BACKREST_ANGLE_2_MOVE,
            ID_SEAT_HEIGHT_POS,
            ID_SEAT_HEIGHT_MOVE,
            ID_SEAT_DEPTH_POS,
            ID_SEAT_DEPTH_MOVE,
            ID_SEAT_TILT_POS,
            ID_SEAT_TILT_MOVE,
            ID_SEAT_LUMBAR_FORE_AFT_POS,
            ID_SEAT_LUMBAR_FORE_AFT_MOVE,
            ID_SEAT_LUMBAR_SIDE_SUPPORT_POS,
            ID_SEAT_LUMBAR_SIDE_SUPPORT_MOVE,
            ID_SEAT_HEADREST_HEIGHT_POS,
            ID_SEAT_HEADREST_HEIGHT_MOVE,
            ID_SEAT_HEADREST_ANGLE_POS,
            ID_SEAT_HEADREST_ANGLE_MOVE,
            ID_SEAT_HEADREST_FORE_AFT_POS,
            ID_SEAT_HEADREST_FORE_AFT_MOVE,
            ID_WINDOW_POS,
            ID_WINDOW_MOVE,
            ID_WINDOW_LOCK
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface PropertyId {}
    private final ArraySet<Integer> mCabinPropertyIds = new ArraySet<>(Arrays.asList(new Integer[]{
            ID_DOOR_POS,
            ID_DOOR_MOVE,
            ID_DOOR_LOCK,
            ID_MIRROR_Z_POS,
            ID_MIRROR_Z_MOVE,
            ID_MIRROR_Y_POS,
            ID_MIRROR_Y_MOVE,
            ID_MIRROR_LOCK,
            ID_MIRROR_FOLD,
            ID_SEAT_MEMORY_SELECT,
            ID_SEAT_MEMORY_SET,
            ID_SEAT_BELT_BUCKLED,
            ID_SEAT_BELT_HEIGHT_POS,
            ID_SEAT_BELT_HEIGHT_MOVE,
            ID_SEAT_FORE_AFT_POS,
            ID_SEAT_FORE_AFT_MOVE,
            ID_SEAT_BACKREST_ANGLE_1_POS,
            ID_SEAT_BACKREST_ANGLE_1_MOVE,
            ID_SEAT_BACKREST_ANGLE_2_POS,
            ID_SEAT_BACKREST_ANGLE_2_MOVE,
            ID_SEAT_HEIGHT_POS,
            ID_SEAT_HEIGHT_MOVE,
            ID_SEAT_DEPTH_POS,
            ID_SEAT_DEPTH_MOVE,
            ID_SEAT_TILT_POS,
            ID_SEAT_TILT_MOVE,
            ID_SEAT_LUMBAR_FORE_AFT_POS,
            ID_SEAT_LUMBAR_FORE_AFT_MOVE,
            ID_SEAT_LUMBAR_SIDE_SUPPORT_POS,
            ID_SEAT_LUMBAR_SIDE_SUPPORT_MOVE,
            ID_SEAT_HEADREST_HEIGHT_POS,
            ID_SEAT_HEADREST_HEIGHT_MOVE,
            ID_SEAT_HEADREST_ANGLE_POS,
            ID_SEAT_HEADREST_ANGLE_MOVE,
            ID_SEAT_HEADREST_FORE_AFT_POS,
            ID_SEAT_HEADREST_FORE_AFT_MOVE,
            ID_WINDOW_POS,
            ID_WINDOW_MOVE,
            ID_WINDOW_LOCK
    }));

    /**
     * Application registers CarCabinEventCallback object to receive updates and changes to
     * subscribed Car Cabin properties.
     */
    public interface CarCabinEventCallback {
        /**
         * Called when a property is updated
         * @param value Property that has been updated.
         */
        void onChangeEvent(CarPropertyValue value);

        /**
         * Called when an error is detected with a property
         * @param propertyId
         * @param zone
         */
        void onErrorEvent(@PropertyId int propertyId, int zone);
    }

    private static class CarPropertyEventListenerToBase implements
            CarPropertyManager.CarPropertyEventListener{
        private final WeakReference<CarCabinManager> mManager;

        public CarPropertyEventListenerToBase(CarCabinManager manager) {
            mManager = new WeakReference<>(manager);
        }

        @Override
        public void onChangeEvent(CarPropertyValue value) {
            CarCabinManager manager = mManager.get();
            if (manager != null) {
                manager.handleOnChangeEvent(value);
            }
        }

        @Override
        public void onErrorEvent(int propertyId, int zone) {
            CarCabinManager manager = mManager.get();
            if (manager != null) {
                manager.handleOnErrorEvent(propertyId, zone);
            }
        }
    }

    private void handleOnChangeEvent(CarPropertyValue value) {
        Collection<CarCabinEventCallback> callbacks;
        synchronized (this) {
            callbacks = new ArraySet<>(mCallbacks);
        }
        for (CarCabinEventCallback l: callbacks) {
            l.onChangeEvent(value);
        }
    }

    private void handleOnErrorEvent(int propertyId, int zone) {
        Collection<CarCabinEventCallback> listeners;
        synchronized (this) {
            listeners = new ArraySet<>(mCallbacks);
        }
        if (!listeners.isEmpty()) {
            for (CarCabinEventCallback l: listeners) {
                l.onErrorEvent(propertyId, zone);
            }
        }
    }

    /**
     * Get an instance of CarCabinManager
     *
     * Should not be obtained directly by clients, use {@link Car#getCarManager(String)} instead.
     * @param service
     * @param context
     * @param handler
     * @hide
     */
    public CarCabinManager(IBinder service, Context context, Handler handler) {
        mCarPropertyMgr = new CarPropertyManager(service, handler, DBG, TAG);
    }

    /**
     * All properties in CarCabinManager are zoned.
     * @param propertyId
     * @return true if property is a zoned type
     */
    public static boolean isZonedProperty(@PropertyId int propertyId) {
        return true;
    }

    /**
     * Implement wrappers for contained CarPropertyManagerBase object
     * @param callback
     * @throws CarNotConnectedException
     */
    public synchronized void registerCallback(CarCabinEventCallback callback) throws
            CarNotConnectedException {
        if (mCallbacks.isEmpty()) {
            mListenerToBase = new CarPropertyEventListenerToBase(this);
        }
        List<CarPropertyConfig> configs = getPropertyList();
        for (CarPropertyConfig c : configs) {
            // Register each individual propertyId
            mCarPropertyMgr.registerListener(mListenerToBase, c.getPropertyId(), 0);
        }
        mCallbacks.add(callback);


    }

    /**
     * Stop getting property updates for the given callback. If there are multiple registrations for
     * this listener, all listening will be stopped.
     * @param callback
     */
    public synchronized void unregisterCallback(CarCabinEventCallback callback)
            throws CarNotConnectedException {
        mCallbacks.remove(callback);
        List<CarPropertyConfig> configs = getPropertyList();
        for (CarPropertyConfig c : configs) {
            // Register each individual propertyId
            mCarPropertyMgr.unregisterListener(mListenerToBase, c.getPropertyId());
        }
        if (mCallbacks.isEmpty()) {
            mListenerToBase = null;
        }
    }

    /**
     * Get list of properties represented by CarCabinManager for this car.
     * @return List of CarPropertyConfig objects available via Car Cabin Manager.
     * @throws CarNotConnectedException if the connection to the car service has been lost.
     */
    public List<CarPropertyConfig> getPropertyList() throws CarNotConnectedException {
        return mCarPropertyMgr.getPropertyList(mCabinPropertyIds);
    }

    /**
     * Get value of boolean property
     * @param propertyId
     * @param area
     * @return value of requested boolean property
     * @throws CarNotConnectedException
     */
    public boolean getBooleanProperty(@PropertyId int propertyId, int area)
            throws CarNotConnectedException {
        return mCarPropertyMgr.getBooleanProperty(propertyId, area);
    }

    /**
     * Get value of float property
     * @param propertyId
     * @param area
     * @return value of requested float property
     * @throws CarNotConnectedException
     */
    public float getFloatProperty(@PropertyId int propertyId, int area)
            throws CarNotConnectedException {
        return mCarPropertyMgr.getFloatProperty(propertyId, area);
    }

    /**
     * Get value of integer property
     * @param propertyId
     * @param area
     * @return value of requested integer property
     * @throws CarNotConnectedException
     */
    public int getIntProperty(@PropertyId int propertyId, int area)
            throws CarNotConnectedException {
        return mCarPropertyMgr.getIntProperty(propertyId, area);
    }

    /**
     * Set the value of a boolean property
     * @param propertyId
     * @param area
     * @param val
     * @throws CarNotConnectedException
     */
    public void setBooleanProperty(@PropertyId int propertyId, int area, boolean val)
            throws CarNotConnectedException {
        if (mCabinPropertyIds.contains(propertyId)) {
            mCarPropertyMgr.setBooleanProperty(propertyId, area, val);
        }
    }

    /**
     * Set the value of a float property
     * @param propertyId
     * @param area
     * @param val
     * @throws CarNotConnectedException
     */
    public void setFloatProperty(@PropertyId int propertyId, int area, float val)
            throws CarNotConnectedException {
        if (mCabinPropertyIds.contains(propertyId)) {
            mCarPropertyMgr.setFloatProperty(propertyId, area, val);
        }
    }

    /**
     * Set the value of an integer property
     * @param propertyId
     * @param area
     * @param val
     * @throws CarNotConnectedException
     */
    public void setIntProperty(@PropertyId int propertyId, int area, int val)
            throws CarNotConnectedException {
        if (mCabinPropertyIds.contains(propertyId)) {
            mCarPropertyMgr.setIntProperty(propertyId, area, val);
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        mCarPropertyMgr.onCarDisconnected();
    }
}
