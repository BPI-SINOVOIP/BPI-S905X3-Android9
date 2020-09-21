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

package android.telephony.euicc.cts;

import android.app.Activity;
import android.app.PendingIntent;
import android.telephony.euicc.EuiccManager;

/**
 * A dummy activity which simulates a resolution activity. Returns {@link Activity#RESULT_OK} to
 * caller if 1) {@link Activity#onResume()} is called (as proof that the activity has been
 * successfully started), and 2) the callback intent is verified.
 */
public class EuiccResolutionActivity extends Activity {

    @Override
    protected void onResume() {
        super.onResume();

        // verify callback intent
        // TODO: verify callback intent's action matches
        // EuiccTestResolutionActivity.ACTION_START_RESOLUTION_ACTIVITY
        PendingIntent callbackIntent =
                getIntent()
                        .getParcelableExtra(
                                EuiccManager
                                        .EXTRA_EMBEDDED_SUBSCRIPTION_RESOLUTION_CALLBACK_INTENT);
        if (callbackIntent != null) {
            setResult(RESULT_OK);
        } else {
            setResult(RESULT_CANCELED);
        }

        // Resolution activity has successfully started, return result & finish
        finish();
    }
}
