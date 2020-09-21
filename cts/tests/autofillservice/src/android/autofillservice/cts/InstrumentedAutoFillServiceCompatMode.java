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

package android.autofillservice.cts;
import android.service.autofill.AutofillService;

/**
 * Implementation of {@link AutofillService} using A11Y compat mode used in the tests.
 */
public class InstrumentedAutoFillServiceCompatMode extends InstrumentedAutoFillService {

    @SuppressWarnings("hiding")
    static final String SERVICE_PACKAGE = "android.autofillservice.cts";
    @SuppressWarnings("hiding")
    static final String SERVICE_CLASS = "InstrumentedAutoFillServiceCompatMode";
    @SuppressWarnings("hiding")
    static final String SERVICE_NAME = SERVICE_PACKAGE + "/." + SERVICE_CLASS;

    public InstrumentedAutoFillServiceCompatMode() {
        sInstance.set(this);
        sServiceLabel = SERVICE_CLASS;
    }
}
