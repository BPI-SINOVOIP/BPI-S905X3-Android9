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
package android.autofillservice.cts;

import android.os.CancellationSignal;
import android.service.autofill.AutofillService;
import android.service.autofill.FillCallback;
import android.service.autofill.FillRequest;
import android.service.autofill.SaveCallback;
import android.service.autofill.SaveRequest;
import android.util.Log;

/**
 * An {@link AutofillService} implementation that does fails if called upon.
 */
public class BadAutofillService extends AutofillService {

    private static final String TAG = "BadAutofillService";

    static final String SERVICE_NAME = BadAutofillService.class.getPackage().getName()
            + "/." + BadAutofillService.class.getSimpleName();
    static final String SERVICE_LABEL = "BadAutofillService";

    @Override
    public void onFillRequest(FillRequest request, CancellationSignal cancellationSignal,
            FillCallback callback) {
        Log.e(TAG, "onFillRequest() should never be called");
        throw new UnsupportedOperationException("onFillRequest() should never be called");
    }

    @Override
    public void onSaveRequest(SaveRequest request, SaveCallback callback) {
        Log.e(TAG, "onSaveRequest() should never be called");
        throw new UnsupportedOperationException("onSaveRequest() should never be called");
    }
}
