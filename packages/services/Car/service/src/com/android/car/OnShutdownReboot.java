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

package com.android.car;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BiConsumer;

/**
 * This class allows one to register actions they want executed when the vehicle is being shutdown
 * or rebooted.
 *
 * To use this class instantiate it as part of your long-lived service, and then add actions to it.
 * Actions receive the Context and Intent that go with the shutdown/reboot action, which allows the
 * action to differentiate the two cases, should it need to do so.
 *
 * The actions will run on the UI thread.
 */
class OnShutdownReboot {
    private final Object mLock = new Object();

    private final Context mContext;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            for (BiConsumer<Context, Intent> action : mActions) {
                action.accept(context, intent);
            }
        }
    };

    private final CopyOnWriteArrayList<BiConsumer<Context, Intent>> mActions =
            new CopyOnWriteArrayList<>();

    OnShutdownReboot(Context context) {
        mContext = context;
        IntentFilter shutdownFilter = new IntentFilter(Intent.ACTION_SHUTDOWN);
        IntentFilter rebootFilter = new IntentFilter(Intent.ACTION_REBOOT);
        mContext.registerReceiver(mReceiver, shutdownFilter);
        mContext.registerReceiver(mReceiver, rebootFilter);
    }

    OnShutdownReboot addAction(BiConsumer<Context, Intent> action) {
        mActions.add(action);
        return this;
    }

    void clearActions() {
        mActions.clear();
    }
}
