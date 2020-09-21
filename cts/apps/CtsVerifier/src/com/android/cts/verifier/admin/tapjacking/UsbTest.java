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

package com.android.cts.verifier.admin.tapjacking;

import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Toast;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

public class UsbTest extends PassFailButtons.Activity {
    private View mOverlay;
    private Button mTriggerOverlayButton;

    public static final String LOG_TAG = "UsbTest";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.tapjacking);
        setPassFailButtonClickListeners();
        setInfoResources(R.string.usb_tapjacking_test,
                R.string.usb_tapjacking_test_info, -1);

        //initialise the escalate button and set a listener
        mTriggerOverlayButton = (Button) findViewById(R.id.tapjacking_btn);
        mTriggerOverlayButton.setEnabled(true);
        mTriggerOverlayButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!Settings.canDrawOverlays(v.getContext())) {
                    // show settings permission
                    startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION));
                }

                if (!Settings.canDrawOverlays(v.getContext())) {
                    Toast.makeText(v.getContext(), R.string.usb_tapjacking_error_toast2,
                            Toast.LENGTH_LONG).show();
                    return;
                }
                showOverlay();
            }
        });
    }

    private void showOverlay() {
        if (mOverlay != null)
            return;

        WindowManager windowManager = (WindowManager) getApplicationContext().
                getSystemService(WINDOW_SERVICE);
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                        | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                        | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED
        );
        layoutParams.format = PixelFormat.TRANSLUCENT;
        layoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT;
        layoutParams.height = ViewGroup.LayoutParams.WRAP_CONTENT;
        layoutParams.x = 0;
        layoutParams.y = dipToPx(-46);
        layoutParams.gravity = Gravity.CENTER;
        layoutParams.windowAnimations = 0;

        mOverlay = View.inflate(getApplicationContext(), R.layout.usb_tapjacking_overlay,
                null);
        windowManager.addView(mOverlay, layoutParams);
    }

    private void hideOverlay() {
        if (mOverlay != null) {
            WindowManager windowManager = (WindowManager) getApplicationContext().getSystemService(
                    WINDOW_SERVICE);
            windowManager.removeViewImmediate(mOverlay);
            mOverlay = null;
        }
    }

    private int dipToPx(int dip) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip,
                getResources().getDisplayMetrics()));
    }

    @Override
    public void onStop() {
        super.onStop();
        hideOverlay();
    }
}
