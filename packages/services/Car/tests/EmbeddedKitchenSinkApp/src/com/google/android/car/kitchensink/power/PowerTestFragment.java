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

package com.google.android.car.kitchensink.power;

import android.car.CarNotConnectedException;
import android.car.hardware.power.CarPowerManager;
import android.content.Context;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.SystemClock;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import java.util.concurrent.Executor;

public class PowerTestFragment extends Fragment {
    private final boolean DBG = false;
    private final String TAG = "PowerTestFragment";
    private CarPowerManager mCarPowerManager;
    private TextView mTvBootReason;
    private Executor mExecutor;

    private class ThreadPerTaskExecutor implements Executor {
        public void execute(Runnable r) {
            new Thread(r).start();
        }
    }

    private final CarPowerManager.CarPowerStateListener mPowerListener =
            new CarPowerManager.CarPowerStateListener () {
                @Override
                public void onStateChanged(int state) {
                    Log.i(TAG, "onStateChanged() state = " + state);
                }
            };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mCarPowerManager = ((KitchenSinkActivity)getActivity()).getPowerManager();
        mExecutor = new ThreadPerTaskExecutor();
        super.onCreate(savedInstanceState);
        try {
            mCarPowerManager.setListener(mPowerListener, mExecutor);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Car is not connected!");
        } catch (IllegalStateException e) {
            Log.e(TAG, "CarPowerManager listener was not cleared");
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mCarPowerManager.clearListener();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstance) {
        View v = inflater.inflate(R.layout.power_test, container, false);

        Button b = v.findViewById(R.id.btnPwrGetBootReason);
        b.setOnClickListener(this::getBootReasonBtn);

        b = v.findViewById(R.id.btnPwrRequestShutdown);
        b.setOnClickListener(this::requestShutdownBtn);

        b = v.findViewById(R.id.btnPwrShutdown);
        b.setOnClickListener(this::shutdownBtn);

        b = v.findViewById(R.id.btnPwrSleep);
        b.setOnClickListener(this::sleepBtn);

        mTvBootReason = v.findViewById(R.id.tvPowerBootReason);

        if(DBG) {
            Log.d(TAG, "Starting PowerTestFragment");
        }

        return v;
    }

    private void getBootReasonBtn(View v) {
        try {
            int bootReason = mCarPowerManager.getBootReason();
            mTvBootReason.setText(String.valueOf(bootReason));
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to getBootReason()", e);
        }
    }

    private void requestShutdownBtn(View v) {
        try {
            mCarPowerManager.requestShutdownOnNextSuspend();
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to set requestShutdownOnNextSuspend()", e);
        }
    }

    private void shutdownBtn(View v) {
        if(DBG) {
            Log.d(TAG, "Calling shutdown method");
        }
        PowerManager pm = (PowerManager) getActivity().getSystemService(Context.POWER_SERVICE);
        pm.shutdown(/* confirm */ false, /* reason */ null, /* wait */ false);
        Log.d(TAG, "shutdown called!");
    }

    private void sleepBtn(View v) {
        if(DBG) {
            Log.d(TAG, "Calling sleep method");
        }
        // NOTE:  This doesn't really work to sleep the device.  Actual sleep is implemented via
        //  SystemInterface via libsuspend::force_suspend()
        PowerManager pm = (PowerManager) getActivity().getSystemService(Context.POWER_SERVICE);
        pm.goToSleep(SystemClock.uptimeMillis(), PowerManager.GO_TO_SLEEP_REASON_DEVICE_ADMIN,
                     PowerManager.GO_TO_SLEEP_FLAG_NO_DOZE);
    }
}
