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

package android.view.cts;

import android.app.Activity;
import android.view.KeyEvent;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;

public class KeyEventInterceptTestActivity extends Activity {
    final BlockingQueue<KeyEvent> mKeyEvents = new LinkedBlockingDeque<>();

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        mKeyEvents.add(event);
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        // Check this in case some spurious event with ACTION_UP is received
        mKeyEvents.add(event);
        return true;
    }
}
