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

package android.car.hardware.power;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.util.concurrent.Executor;

import com.android.internal.annotations.GuardedBy;

import android.annotation.IntDef;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.content.Context;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;

/**
 * API for receiving power state change notifications.
 * @hide
 */
@SystemApi
public class CarPowerManager implements CarManagerBase {
    private final static boolean DBG = false;
    private final static String TAG = "CarPowerManager";
    private CarPowerStateListener mListener;
    private final ICarPower mService;
    private Executor mExecutor;

    @GuardedBy("mLock")
    private ICarPowerStateListener mListenerToService;

    private final Object mLock = new Object();

    /**
     * Power boot up reasons, returned by {@link getBootReason}
     */
    /**
     * User powered on the vehicle.  These definitions must match the ones located in the native
     * CarPowerManager:  packages/services/Car/car-lib/native/CarPowerManager/CarPowerManager.h
     *
     */
    public static final int BOOT_REASON_USER_POWER_ON = 1;
    /**
     * Door unlock caused device to boot
     */
    public static final int BOOT_REASON_DOOR_UNLOCK = 2;
    /**
     * Timer expired and vehicle woke up the AP
     */
    public static final int BOOT_REASON_TIMER = 3;
    /**
     * Door open caused device to boot
     */
    public static final int BOOT_REASON_DOOR_OPEN = 4;
    /**
     * User activated remote start
     */
    public static final int BOOT_REASON_REMOTE_START = 5;

    /** @hide */
    @IntDef({
        BOOT_REASON_USER_POWER_ON,
        BOOT_REASON_DOOR_UNLOCK,
        BOOT_REASON_TIMER,
        BOOT_REASON_DOOR_OPEN,
        BOOT_REASON_REMOTE_START,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface BootReason{}

    /**
     *  Applications set a {@link CarPowerStateListener} for power state event updates.
     */
    public interface CarPowerStateListener {
        /**
         * onStateChanged() states.  These definitions must match the ones located in the native
         * CarPowerManager:  packages/services/Car/car-lib/native/CarPowerManager/CarPowerManager.h
         *
         */
        /**
         * Shutdown is cancelled, return to normal state.
         */
        public static final int SHUTDOWN_CANCELLED = 0;
        /**
         * Enter shutdown state.  Application is expected to cleanup and be ready to shutdown.
         */
        public static final int SHUTDOWN_ENTER = 1;
        /**
         * Enter suspend state.  Application is expected to cleanup and be ready to suspend.
         */
        public static final int SUSPEND_ENTER = 2;
        /**
         * Wake up from suspend, or resume from a cancelled suspend.  Application transitions to
         * normal state.
         */
        public static final int SUSPEND_EXIT = 3;

        /**
         *  Called when power state changes
         *  @param state New power state of device.
         *  @param token Opaque identifier to keep track of listener events.
         */
        void onStateChanged(int state);
    }

    /**
     * Get an instance of the CarPowerManager.
     *
     * Should not be obtained directly by clients, use {@link Car#getCarManager(String)} instead.
     * @param service
     * @param context
     * @param handler
     * @hide
     */
    public CarPowerManager(IBinder service, Context context, Handler handler) {
        mService = ICarPower.Stub.asInterface(service);
    }

    /**
     * Returns the current {@link BootReason}.  This value does not change until the device goes
     * through a suspend/resume cycle.
     * @return int
     * @throws CarNotConnectedException
     * @hide
     */
    public int getBootReason() throws CarNotConnectedException {
        try {
            return mService.getBootReason();
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getBootReason", e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Request power manager to shutdown in lieu of suspend at the next opportunity.
     * @throws CarNotConnectedException
     * @hide
     */
    public void requestShutdownOnNextSuspend() throws CarNotConnectedException {
        try {
            mService.requestShutdownOnNextSuspend();
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in requestShutdownOnNextSuspend", e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Sets a listener to receive power state changes.  Only one listener may be set at a time.
     * Caller may add an Executor to allow the callback to run in a seperate thread of execution
     * if the {@link onStateChanged} method will take some time.  If no Executor is passed in,
     * the listener will run in the Binder thread and should finish quickly.  After
     * {@link onStateChanged} is called, the {@link finished} method will automatically be called
     * to notify {@link CarPowerManagementService} that the application has handled the
     * {@link #SHUTDOWN_ENTER} or {@link #SUSPEND_ENTER} state transitions.  Only these two states
     * require a confirmation from the application.
     *
     * @param executor
     * @param listener
     * @throws CarNotConnectedException, IllegalStateException
     * @hide
     */
    public void setListener(CarPowerStateListener listener, Executor executor) throws
            CarNotConnectedException, IllegalStateException {
        synchronized(mLock) {
            if (mListenerToService == null) {
                ICarPowerStateListener listenerToService = new ICarPowerStateListener.Stub() {
                    @Override
                    public void onStateChanged(int state, int token) throws RemoteException {
                        handleEvent(state, token);
                    }
                };
                try {
                    mService.registerListener(listenerToService);
                    mListenerToService = listenerToService;
                } catch (RemoteException ex) {
                    Log.e(TAG, "Could not connect: ", ex);
                    throw new CarNotConnectedException(ex);
                } catch (IllegalStateException ex) {
                    Car.checkCarNotConnectedExceptionFromCarService(ex);
                }
            }
            if ((mExecutor == null) && (mListener == null)) {
                // Update listener and executor
                mExecutor = executor;
                mListener = listener;
            } else {
                throw new IllegalStateException("Listener must be cleared first");
            }
        }
    }

    /**
     * Removes the listener from {@link CarPowerManagementService}
     * @hide
     */
    public void clearListener() {
        ICarPowerStateListener listenerToService;
        synchronized (mLock) {
            listenerToService = mListenerToService;
            mListenerToService = null;
            mListener = null;
            mExecutor = null;
        }

        if (listenerToService == null) {
            Log.w(TAG, "unregisterListener: listener was not registered");
            return;
        }

        try {
            mService.unregisterListener(listenerToService);
        } catch (RemoteException ex) {
            Log.e(TAG, "Failed to unregister listener", ex);
            //ignore
        } catch (IllegalStateException ex) {
            Car.hideCarNotConnectedExceptionFromCarService(ex);
        }
    }

    private void handleEvent(int state, int token) {
        Executor executor;
        synchronized (mLock) {
            executor = mExecutor;
        }
        if (executor != null) {
            executor.execute(() -> {
                handleEventInternal(state, token);
            });
        } else {
            // If no executor provided, run in binder thread.  This should only be done for
            //  trivial listener logic.
            handleEventInternal(state, token);
        }
    }

    private void handleEventInternal(int state, int token) {
        mListener.onStateChanged(state);
        if ((state == CarPowerStateListener.SHUTDOWN_ENTER) ||
            (state == CarPowerStateListener.SUSPEND_ENTER)) {
            // Notify service that state change is complete for SHUTDOWN_ENTER and SUSPEND_ENTER
            //  states only.
            try {
                mService.finished(mListenerToService, token);
            } catch (RemoteException e) {
                Log.e(TAG, "Exception in finished", e);
            }
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        ICarPowerStateListener listenerToService;
        synchronized (mLock) {
            listenerToService = mListenerToService;
        }

        if (listenerToService != null) {
            clearListener();
        }
    }
}
