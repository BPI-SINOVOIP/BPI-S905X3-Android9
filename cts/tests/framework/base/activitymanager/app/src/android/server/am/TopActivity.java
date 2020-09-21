/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.server.am;

import static android.server.am.Components.TopActivity.EXTRA_FINISH_DELAY;
import static android.server.am.Components.TopActivity.EXTRA_TOP_WALLPAPER;

import android.os.Handler;
import android.os.Bundle;
import android.util.Log;

public class TopActivity extends AbstractLifecycleLogActivity {

    protected void setWallpaperTheme() {
        setTheme(R.style.WallpaperTheme);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final boolean useWallpaper = getIntent().getBooleanExtra(EXTRA_TOP_WALLPAPER, false);
        if (useWallpaper) {
            setWallpaperTheme();
        }

        final int finishDelay = getIntent().getIntExtra(EXTRA_FINISH_DELAY, 0);
        if (finishDelay > 0) {
            Handler handler = new Handler();
            handler.postDelayed(() -> {
                    Log.d(getTag(), "Calling finish()");
                    finish();
            }, finishDelay);
        }
    }
}
