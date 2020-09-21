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

package com.mediatek.tunerservice;

interface IMtkTuner {
    boolean tune(int frequency, String modulation, int timeOutMs);
    void addPidFilter(int pid, int filterType);
    void closeAllPidFilters();
    void stopTune();
    byte[] getTsData(int maxDataSize, int timeOutMs);
    void setHasPendingTune(boolean hasPendingTune);
    void release();
}
