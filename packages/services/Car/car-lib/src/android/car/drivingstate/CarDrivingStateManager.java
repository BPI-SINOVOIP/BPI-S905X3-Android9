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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.car.drivingstate.ICarDrivingState;
import android.content.Context;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import java.lang.ref.WeakReference;

/**
 * API to register and get driving state related information in a car.
 * @hide
 */
@SystemApi
public final class CarDrivingStateManager implements CarManagerBase {
    private static final String TAG = "CarDrivingStateMgr";
    private static final boolean DBG = false;
    private static final boolean VDBG = false;
    private static final int MSG_HANDLE_DRIVING_STATE_CHANGE = 0;

    private final Context mContext;
    private final ICarDrivingState mDrivingService;
    private final EventCallbackHandler mEventCallbackHandler;
    private CarDrivingStateEventListener mDrvStateEventListener;
    private CarDrivingStateChangeListenerToService mListenerToService;


    /** @hide */
    public CarDrivingStateManager(IBinder service, Context context, Handler handler) {
        mContext = context;
        mDrivingService = ICarDrivingState.Stub.asInterface(service);
        mEventCallbackHandler = new EventCallbackHandler(this, handler.getLooper());
    }

    /** @hide */
    @Override
    public synchronized void onCarDisconnected() {
            mListenerToService = null;
            mDrvStateEventListener = null;
    }

    /**
     * Listener Interface for clients to implement to get updated on driving state changes.
     */
    public interface CarDrivingStateEventListener {
        /**
         * Called when the car's driving state changes.
         * @param event Car's driving state.
         */
        void onDrivingStateChanged(CarDrivingStateEvent event);
    }

    /**
     * Register a {@link CarDrivingStateEventListener} to listen for driving state changes.
     *
     * @param listener  {@link CarDrivingStateEventListener}
     */
    public synchronized void registerListener(@NonNull CarDrivingStateEventListener listener)
            throws CarNotConnectedException, IllegalArgumentException {
        if (listener == null) {
            if (VDBG) {
                Log.v(TAG, "registerCarDrivingStateEventListener(): null listener");
            }
            throw new IllegalArgumentException("Listener is null");
        }
        // Check if the listener has been already registered for this event type
        if (mDrvStateEventListener != null) {
            if (DBG) {
                Log.d(TAG, "Listener already registered");
            }
            return;
        }
        mDrvStateEventListener = listener;
        try {
            if (mListenerToService == null) {
                mListenerToService = new CarDrivingStateChangeListenerToService(this);
            }
            // register to the Service for getting notified
            mDrivingService.registerDrivingStateChangeListener(mListenerToService);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not register a listener to Driving State Service " + e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "Could not register a listener to Driving State Service " + e);
            Car.checkCarNotConnectedExceptionFromCarService(e);
        }
    }

    /**
     * Unregister the registered {@link CarDrivingStateEventListener} for the given driving event
     * type.
     */
    public synchronized void unregisterListener()
            throws CarNotConnectedException {
        if (mDrvStateEventListener == null) {
            if (DBG) {
                Log.d(TAG, "Listener was not previously registered");
            }
            return;
        }
        try {
            mDrivingService.unregisterDrivingStateChangeListener(mListenerToService);
            mDrvStateEventListener = null;
            mListenerToService = null;
        } catch (RemoteException e) {
            Log.e(TAG, "Could not unregister listener from Driving State Service " + e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Get the current value of the car's driving state.
     *
     * @return {@link CarDrivingStateEvent} corresponding to the given eventType
     */
    @Nullable
    public CarDrivingStateEvent getCurrentCarDrivingState()
            throws CarNotConnectedException {
        try {
            return mDrivingService.getCurrentDrivingState();
        } catch (RemoteException e) {
            Log.e(TAG, "Could not get current driving state " + e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Class that implements the listener interface and gets called back from the
     * {@link com.android.car.CarDrivingStateService} across the binder interface.
     */
    private static class CarDrivingStateChangeListenerToService extends
            ICarDrivingStateChangeListener.Stub {
        private final WeakReference<CarDrivingStateManager> mDrvStateMgr;

        public CarDrivingStateChangeListenerToService(CarDrivingStateManager manager) {
            mDrvStateMgr = new WeakReference<>(manager);
        }

        @Override
        public void onDrivingStateChanged(CarDrivingStateEvent event) {
            CarDrivingStateManager manager = mDrvStateMgr.get();
            if (manager != null) {
                manager.handleDrivingStateChanged(event);
            }
        }
    }

    /**
     * Gets the {@link CarDrivingStateEvent} from the service listener
     * {@link CarDrivingStateChangeListenerToService} and dispatches it to a handler provided
     * to the manager
     *
     * @param event  {@link CarDrivingStateEvent} that has been registered to listen on
     */
    private void handleDrivingStateChanged(CarDrivingStateEvent event) {
        // send a message to the handler
        mEventCallbackHandler.sendMessage(
                mEventCallbackHandler.obtainMessage(MSG_HANDLE_DRIVING_STATE_CHANGE, event));

    }

    /**
     * Callback Handler to handle dispatching the driving state changes to the corresponding
     * listeners
     */
    private static final class EventCallbackHandler extends Handler {
        private final WeakReference<CarDrivingStateManager> mDrvStateMgr;

        public EventCallbackHandler(CarDrivingStateManager manager, Looper looper) {
            super(looper);
            mDrvStateMgr = new WeakReference<>(manager);
        }

        @Override
        public void handleMessage(Message msg) {
            CarDrivingStateManager mgr = mDrvStateMgr.get();
            if (mgr != null) {
                mgr.dispatchDrivingStateChangeToClient((CarDrivingStateEvent) msg.obj);
            }
        }

    }

    /**
     * Checks for the listener to {@link CarDrivingStateEvent} and calls it back
     * in the callback handler thread
     *
     * @param event  {@link CarDrivingStateEvent}
     */
    private void dispatchDrivingStateChangeToClient(CarDrivingStateEvent event) {
        if (event == null) {
            return;
        }
        CarDrivingStateEventListener listener;
        synchronized (this) {
            listener = mDrvStateEventListener;
        }
        if (listener != null) {
            listener.onDrivingStateChanged(event);
        }
    }

}
