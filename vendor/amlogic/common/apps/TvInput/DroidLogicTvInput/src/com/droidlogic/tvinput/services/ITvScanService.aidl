/*
 * Copyright (C) 2017 Amlogic Corporation.
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


package com.droidlogic.tvinput.services;

import android.content.Intent;
import com.droidlogic.tvinput.services.IUpdateUiCallbackListener;

interface ITvScanService{
    void init(in Intent intent);

    void setAtsccSearchSys (int value);

    void startAutoScan();

    void startManualScan();

    void setSearchSys(boolean value1, boolean value2);

    void setFrequency(String value1, String value2);

    void release();

    void registerListener(IUpdateUiCallbackListener listener);

    void unregisterListener(IUpdateUiCallbackListener listener);
}
