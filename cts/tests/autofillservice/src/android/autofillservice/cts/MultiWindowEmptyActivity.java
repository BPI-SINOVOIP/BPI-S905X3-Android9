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

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Empty activity that allows to be put in different split window.
 */
public class MultiWindowEmptyActivity extends EmptyActivity {

    private static MultiWindowEmptyActivity sLastInstance;
    private static CountDownLatch sLastInstanceLatch;

    @Override
    protected void onStart() {
        super.onStart();
        sLastInstance = this;
        if (sLastInstanceLatch != null) {
            sLastInstanceLatch.countDown();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (hasFocus) {
            if (sLastInstanceLatch != null) {
                sLastInstanceLatch.countDown();
            }
        }
    }

    public static void expectNewInstance(boolean waitWindowFocus) {
        sLastInstanceLatch = new CountDownLatch(waitWindowFocus ? 2 : 1);
    }

    public static MultiWindowEmptyActivity waitNewInstance() throws InterruptedException {
        sLastInstanceLatch.await(Timeouts.ACTIVITY_RESURRECTION.getMaxValue(),
                TimeUnit.MILLISECONDS);
        sLastInstanceLatch = null;
        return sLastInstance;
    }
}
