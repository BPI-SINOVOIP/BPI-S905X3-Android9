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
 * limitations under the License
 */

package android.server.wm;

import android.app.Activity;
import android.view.View;
import android.view.WindowManager;

import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;

public class AlertWindowsAppOpsTestsActivity extends Activity {
    private View mContent;

    public void showSystemAlertWindow() {
        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                TYPE_APPLICATION_OVERLAY);
        params.width = WindowManager.LayoutParams.MATCH_PARENT;
        params.height = WindowManager.LayoutParams.MATCH_PARENT;
        mContent = new View(this);
        getWindowManager().addView(mContent, params);
    }

    public void hideSystemAlertWindow() {
        getWindowManager().removeView(mContent);
    }
}
