/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.phone;

import android.telephony.CellInfo;
import com.android.internal.telephony.OperatorInfo;

/**
 * Service interface to handle callbacks into the activity from the
 * NetworkQueryService.  These objects are used to notify that a
 * query is complete and that the results are ready to process.
 */
oneway interface INetworkQueryServiceCallback {

    /**
     * Returns the scan results to the user, this callback will be
     * called at least one time.
     */
    void onResults(in List<CellInfo> results);

    /**
     * Informs the user that the scan has stopped.
     *
     * This callback will be called when the scan is finished or cancelled by the user.
     * The related NetworkScanRequest will be deleted after this callback.
     */
    void onComplete();

    /**
     * Informs the user that there is some error about the scan.
     *
     * This callback will be called whenever there is any error about the scan,
     * and the scan will be terminated. onComplete() will NOT be called.
     */
    void onError(int error);

}
