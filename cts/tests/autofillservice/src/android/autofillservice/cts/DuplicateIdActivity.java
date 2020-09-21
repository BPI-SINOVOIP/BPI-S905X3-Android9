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

import android.os.Bundle;
import android.util.Log;

public class DuplicateIdActivity extends AbstractAutoFillActivity {
    private static final String LOG_TAG = DuplicateIdActivity.class.getSimpleName();

    static final String DUPLICATE_ID = "duplicate_id";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (savedInstanceState != null) {
            Log.i(LOG_TAG, "onCreate(" + savedInstanceState + ")");
        }

        setContentView(R.layout.duplicate_id_layout);
    }
}
