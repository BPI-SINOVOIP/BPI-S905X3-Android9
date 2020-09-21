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

package android.fragment.cts.sdk26;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;

import java.util.concurrent.CountDownLatch;

public class NonConfigOnStopActivity extends Activity {
    static NonConfigOnStopActivity sActivity;
    static CountDownLatch sDestroyed;
    static CountDownLatch sResumed;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sActivity = this;
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (sResumed != null) {
            sResumed.countDown();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();

        getFragmentManager()
                .beginTransaction()
                .add(new RetainedFragment(), "1")
                .commitNowAllowingStateLoss();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (sDestroyed != null) {
            sDestroyed.countDown();
        }
    }

    public static class RetainedFragment extends Fragment {
        public RetainedFragment() {
            setRetainInstance(true);
        }
    }
}
