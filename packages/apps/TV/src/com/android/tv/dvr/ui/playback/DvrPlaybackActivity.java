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
 * limitations under the License
 */

package com.android.tv.dvr.ui.playback;

import android.app.Activity;
import android.content.ContentUris;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.text.TextUtils;
import android.content.IntentFilter;

import com.android.tv.R;
import com.android.tv.Starter;
import com.android.tv.dialog.PinDialogFragment.OnPinCheckedListener;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.util.Utils;
import com.android.tv.droidlogic.subtitleui.TeleTextAdvancedSettings;

/** Activity to play a {@link RecordedProgram}. */
public class DvrPlaybackActivity extends Activity implements OnPinCheckedListener {
    private static final String TAG = "DvrPlaybackActivity";
    private static final boolean DEBUG = true;

    public static final String SYSTEM_DIALOG_REASON_KEY = "reason";
    public static final String SYSTEM_DIALOG_REASON_HOME_KEY = "homekey";

    private DvrPlaybackOverlayFragment mOverlayFragment;
    private OnPinCheckedListener mOnPinCheckedListener;
    private TeleTextAdvancedSettings mTeleTextAdvancedSettings;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action != null) {
                switch (action) {
                    case Intent.ACTION_CLOSE_SYSTEM_DIALOGS:
                        String reason = intent.getStringExtra(SYSTEM_DIALOG_REASON_KEY);
                        if (TextUtils.equals(reason, SYSTEM_DIALOG_REASON_HOME_KEY)) {
                            finish();
                        }
                        break;
                    case Intent.ACTION_SCREEN_OFF:
                        finish();
                        break;
                    default:
                        break;
                }
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Starter.start(this);
        if (DEBUG) Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        registerMainReceiver();
        setIntent(createProgramIntent(getIntent()));
        setContentView(R.layout.activity_dvr_playback);
        mOverlayFragment =
                (DvrPlaybackOverlayFragment)
                        getFragmentManager().findFragmentById(R.id.dvr_playback_controls_fragment);
        mTeleTextAdvancedSettings = new TeleTextAdvancedSettings(this, mOverlayFragment.getTvView());
    }

    @Override
    public void onVisibleBehindCanceled() {
        if (DEBUG) Log.d(TAG, "onVisibleBehindCanceled");
        super.onVisibleBehindCanceled();
        finish();
    }

    @Override
    public void onStop() {
        super.onStop();
        hideTeleTextAdvancedSettings();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (DEBUG) Log.d(TAG, "onDestroy");
        unregisterMainReceiver();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        setIntent(createProgramIntent(intent));
        mOverlayFragment.onNewIntent(createProgramIntent(intent));
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        float density = getResources().getDisplayMetrics().density;
        mOverlayFragment.onWindowSizeChanged(
                (int) (newConfig.screenWidthDp * density),
                (int) (newConfig.screenHeightDp * density));
    }

    private Intent createProgramIntent(Intent intent) {
        if (Intent.ACTION_VIEW.equals(intent.getAction())) {
            Uri uri = intent.getData();
            long recordedProgramId = ContentUris.parseId(uri);
            intent.putExtra(Utils.EXTRA_KEY_RECORDED_PROGRAM_ID, recordedProgramId);
        }
        return intent;
    }

    @Override
    public void onPinChecked(boolean checked, int type, String rating) {
        if (mOnPinCheckedListener != null) {
            mOnPinCheckedListener.onPinChecked(checked, type, rating);
        }
    }

    void setOnPinCheckListener(OnPinCheckedListener listener) {
        mOnPinCheckedListener = listener;
    }

    public void showTeleTextAdvancedSettings() {
        if (mOverlayFragment != null) {
            mOverlayFragment.hideControlUi();
        }
        if (mTeleTextAdvancedSettings.getTvView() == null) {
            mTeleTextAdvancedSettings.setTvView(mOverlayFragment.getTvView());
        }
        mTeleTextAdvancedSettings.creatTeletextAdvancedDialog();
    }

    public void hideTeleTextAdvancedSettings() {
        if (mTeleTextAdvancedSettings.getTvView() == null) {
            mTeleTextAdvancedSettings.setTvView(mOverlayFragment.getTvView());
        }
        mTeleTextAdvancedSettings.hideTeletextAdvancedDialog();
    }

    private void unregisterMainReceiver() {
        unregisterReceiver(mReceiver);
    }

    private void registerMainReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
        registerReceiver(mReceiver, intentFilter);
    }
}
