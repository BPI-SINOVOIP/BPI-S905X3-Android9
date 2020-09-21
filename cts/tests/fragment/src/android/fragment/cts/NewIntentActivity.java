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
package android.fragment.cts;

import android.app.Activity;
import android.app.Fragment;
import android.content.Intent;
import android.os.Bundle;

import java.util.concurrent.CountDownLatch;

public class NewIntentActivity extends Activity {
    public final CountDownLatch newIntent = new CountDownLatch(1);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState == null) {
            getFragmentManager()
                    .beginTransaction()
                    .add(new FooFragment(), "derp")
                    .commitNow();
        }
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        // Test a child fragment transaction -
        getFragmentManager()
                .findFragmentByTag("derp")
                .getChildFragmentManager()
                .beginTransaction()
                .add(new FooFragment(), "derp4")
                .commitNow();
        newIntent.countDown();
    }

    public static class FooFragment extends Fragment {
    }
}
