/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.droidlogic;
//package com.droidlogic.tv.settings;


import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;


import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.IDolbyAudioEffectService;

public class DolbyAudioEffectService extends Service {
    private static final String TAG = "DolbyAudioEffectService";
    private OutputModeManager mOpm;

    public DolbyAudioEffectService() {
        super();
        Log.d(TAG, "DolbyAudioEffectService");
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mOpm = new OutputModeManager(this);
        mOpm.initDapAudioEffect();
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private final IDolbyAudioEffectService.Stub mBinder = new IDolbyAudioEffectService.Stub(){
        public void setDapParam(int id, int value) {
            mOpm.saveDapParam(id, value);
            mOpm.setDapParam(id, value);
        }

        public int getDapParam(int id) {
            return mOpm.getDapParam(id);
        }

        public void initDapAudioEffect() {
            mOpm.initDapAudioEffect();
        }
    };
}

