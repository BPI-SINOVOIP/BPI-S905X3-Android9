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
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.car.drivingstate.ICarUxRestrictionsManager;
import android.content.Context;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;

import java.lang.ref.WeakReference;

/**
 * API to register and get the User Experience restrictions imposed based on the car's driving
 * state.
 */
public final class CarUxRestrictionsManager implements CarManagerBase {
    private static final String TAG = "CarUxRManager";
    private static final boolean DBG = false;
    private static final boolean VDBG = false;
    private static final int MSG_HANDLE_UX_RESTRICTIONS_CHANGE = 0;

    private final Context mContext;
    private final ICarUxRestrictionsManager mUxRService;
    private final EventCallbackHandler mEventCallbackHandler;
    private OnUxRestrictionsChangedListener mUxRListener;
    private CarUxRestrictionsChangeListenerToService mListenerToService;


    /** @hide */
    public CarUxRestrictionsManager(IBinder service, Context context, Handler handler) {
        mContext = context;
        mUxRService = ICarUxRestrictionsManager.Stub.asInterface(service);
        mEventCallbackHandler = new EventCallbackHandler(this, handler.getLooper());
    }

    /** @hide */
    @Override
    public synchronized void onCarDisconnected() {
        mListenerToService = null;
        mUxRListener = null;
    }

    /**
     * Listener Interface for clients to implement to get updated on driving state related
     * changes.
     */
    public interface OnUxRestrictionsChangedListener {
        /**
         * Called when the UX restrictions due to a car's driving state changes.
         *
         * @param restrictionInfo The new UX restriction information
         */
        void onUxRestrictionsChanged(CarUxRestrictions restrictionInfo);
    }

    /**
     * Register a {@link OnUxRestrictionsChangedListener} for listening to changes in the
     * UX Restrictions to adhere to.
     * <p>
     * If a listener has already been registered, it has to be unregistered before registering
     * the new one.
     *
     * @param listener {@link OnUxRestrictionsChangedListener}
     */
    public synchronized void registerListener(@NonNull OnUxRestrictionsChangedListener listener)
            throws CarNotConnectedException, IllegalArgumentException {
        if (listener == null) {
            if (VDBG) {
                Log.v(TAG, "registerListener(): null listener");
            }
            throw new IllegalArgumentException("Listener is null");
        }
        // Check if the listener has been already registered.
        if (mUxRListener != null) {
            if (DBG) {
                Log.d(TAG, "Listener already registered listener");
            }
            return;
        }
        mUxRListener = listener;
        try {
            if (mListenerToService == null) {
                mListenerToService = new CarUxRestrictionsChangeListenerToService(this);
            }
            // register to the Service to listen for changes.
            mUxRService.registerUxRestrictionsChangeListener(mListenerToService);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not register a listener to CarUxRestrictionsManagerService " + e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "Could not register a listener to CarUxRestrictionsManagerService " + e);
            Car.checkCarNotConnectedExceptionFromCarService(e);
        }
    }

    /**
     * Unregister the registered {@link OnUxRestrictionsChangedListener}
     */
    public synchronized void unregisterListener()
            throws CarNotConnectedException {
        if (mUxRListener == null) {
            if (DBG) {
                Log.d(TAG, "Listener was not previously registered");
            }
            return;
        }
        try {
            mUxRService.unregisterUxRestrictionsChangeListener(mListenerToService);
            mUxRListener = null;
        } catch (RemoteException e) {
            Log.e(TAG, "Could not unregister listener from Driving State Service " + e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Get the current UX restrictions {@link CarUxRestrictions} in place.
     *
     * @return current UX restrictions that is in effect.
     */
    @Nullable
    public CarUxRestrictions getCurrentCarUxRestrictions()
            throws CarNotConnectedException {
        try {
            return mUxRService.getCurrentUxRestrictions();
        } catch (RemoteException e) {
            Log.e(TAG, "Could not get current UX restrictions " + e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Class that implements the listener interface and gets called back from the
     * {@link com.android.car.CarDrivingStateService} across the binder interface.
     */
    private static class CarUxRestrictionsChangeListenerToService extends
            ICarUxRestrictionsChangeListener.Stub {
        private final WeakReference<CarUxRestrictionsManager> mUxRestrictionsManager;

        public CarUxRestrictionsChangeListenerToService(CarUxRestrictionsManager manager) {
            mUxRestrictionsManager = new WeakReference<>(manager);
        }

        @Override
        public void onUxRestrictionsChanged(CarUxRestrictions restrictionInfo) {
            CarUxRestrictionsManager manager = mUxRestrictionsManager.get();
            if (manager != null) {
                manager.handleUxRestrictionsChanged(restrictionInfo);
            }
        }
    }

    /**
     * Gets the {@link CarUxRestrictions} from the service listener
     * {@link CarUxRestrictionsChangeListenerToService} and dispatches it to a handler provided
     * to the manager
     *
     * @param restrictionInfo {@link CarUxRestrictions} that has been registered to listen on
     */
    private void handleUxRestrictionsChanged(CarUxRestrictions restrictionInfo) {
        // send a message to the handler
        mEventCallbackHandler.sendMessage(mEventCallbackHandler.obtainMessage(
                MSG_HANDLE_UX_RESTRICTIONS_CHANGE, restrictionInfo));
    }

    /**
     * Callback Handler to handle dispatching the UX restriction changes to the corresponding
     * listeners
     */
    private static final class EventCallbackHandler extends Handler {
        private final WeakReference<CarUxRestrictionsManager> mUxRestrictionsManager;

        public EventCallbackHandler(CarUxRestrictionsManager manager, Looper looper) {
            super(looper);
            mUxRestrictionsManager = new WeakReference<>(manager);
        }

        @Override
        public void handleMessage(Message msg) {
            CarUxRestrictionsManager mgr = mUxRestrictionsManager.get();
            if (mgr != null) {
                mgr.dispatchUxRChangeToClient((CarUxRestrictions) msg.obj);
            }
        }

    }

    /**
     * Checks for the listeners to list of {@link CarUxRestrictions} and calls them back
     * in the callback handler thread
     *
     * @param restrictionInfo {@link CarUxRestrictions}
     */
    private void dispatchUxRChangeToClient(CarUxRestrictions restrictionInfo) {
        if (restrictionInfo == null) {
            return;
        }
        OnUxRestrictionsChangedListener listener;
        synchronized (this) {
            listener = mUxRListener;
        }
        if (listener != null) {
            listener.onUxRestrictionsChanged(restrictionInfo);
        }
    }
}
