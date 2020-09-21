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
 * limitations under the License
 */

package com.android.tv.testing;

import android.media.tv.TvContentRatingSystemInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Handler;
import com.android.tv.util.TvInputManagerHelper;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Fake implementation for testing. */
public class FakeTvInputManager implements TvInputManagerHelper.TvInputManagerInterface {

    private final Map<String, Integer> mInputStateMap = new HashMap<>();
    private final Map<String, TvInputInfo> mInputMap = new HashMap<>();
    private final Map<TvInputManager.TvInputCallback, Handler> mCallbacks = new HashMap<>();

    public void add(TvInputInfo inputInfo, int state) {
        final String inputId = inputInfo.getId();
        if (mInputStateMap.containsKey(inputId)) {
            throw new IllegalArgumentException("inputId " + inputId);
        }
        mInputMap.put(inputId, inputInfo);
        mInputStateMap.put(inputId, state);
        for (final Map.Entry<TvInputManager.TvInputCallback, Handler> e : mCallbacks.entrySet()) {
            e.getValue()
                    .post(
                            new Runnable() {
                                @Override
                                public void run() {
                                    e.getKey().onInputAdded(inputId);
                                }
                            });
        }
    }

    public void setInputState(final String inputId, final int state) {
        if (!mInputStateMap.containsKey(inputId)) {
            throw new IllegalArgumentException("inputId " + inputId);
        }
        mInputStateMap.put(inputId, state);
        for (final Map.Entry<TvInputManager.TvInputCallback, Handler> e : mCallbacks.entrySet()) {
            e.getValue()
                    .post(
                            new Runnable() {
                                @Override
                                public void run() {
                                    e.getKey().onInputStateChanged(inputId, state);
                                }
                            });
        }
    }

    public void remove(final String inputId) {
        mInputMap.remove(inputId);
        mInputStateMap.remove(inputId);
        for (final Map.Entry<TvInputManager.TvInputCallback, Handler> e : mCallbacks.entrySet()) {
            e.getValue()
                    .post(
                            new Runnable() {
                                @Override
                                public void run() {
                                    e.getKey().onInputRemoved(inputId);
                                }
                            });
        }
    }

    @Override
    public TvInputInfo getTvInputInfo(String inputId) {
        return mInputMap.get(inputId);
    }

    @Override
    public Integer getInputState(String inputId) {
        return mInputStateMap.get(inputId);
    }

    @Override
    public void registerCallback(TvInputManager.TvInputCallback internalCallback, Handler handler) {
        mCallbacks.put(internalCallback, handler);
    }

    @Override
    public void unregisterCallback(TvInputManager.TvInputCallback internalCallback) {
        mCallbacks.remove(internalCallback);
    }

    @Override
    public List<TvInputInfo> getTvInputList() {
        return new ArrayList(mInputMap.values());
    }

    @Override
    public List<TvContentRatingSystemInfo> getTvContentRatingSystemList() {
        // TODO implement
        return new ArrayList<>();
    }
}
