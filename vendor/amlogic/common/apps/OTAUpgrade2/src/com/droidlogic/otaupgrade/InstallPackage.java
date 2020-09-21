/******************************************************************
*
*Copyright(C) 2012 Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.otaupgrade;

import android.content.Context;

import android.os.Handler;

import android.os.RecoverySystem.ProgressListener;

import android.util.AttributeSet;

import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;

import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.LayoutAnimationController;

import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.amlogic.update.OtaUpgradeUtils;

import java.io.File;


public class InstallPackage extends LinearLayout implements OtaUpgradeUtils.ProgressListener {
    private ProgressBar mProgressBar;
    private LinearLayout mOutputField;
    private LayoutInflater mInflater;
    private String mPackagePath;
    private OtaUpgradeUtils mUpdateUtils;
    private Handler mHandler = new Handler();
    private Button mDismiss;
    private int mUpdateMode;
    private PrefUtils mPref = null;
    private boolean isDelUpdate = false;
    public InstallPackage(Context context, AttributeSet attrs) {
        super(context, attrs);
        mInflater = LayoutInflater.from(context);
        mUpdateUtils = new OtaUpgradeUtils(context);
        mUpdateMode = OtaUpgradeUtils.UPDATE_OTA;
        mPref = new PrefUtils(context);
        requestFocus();
    }

    public void setPackagePath(String path) {
        mPackagePath = path;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mProgressBar = (ProgressBar) findViewById(R.id.verify_progress);
        mOutputField = (LinearLayout) findViewById(R.id.output_field);

        final TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                null);
        tv.setText(R.string.install_ota_output_confirm);
        mOutputField.addView(tv);

        Animation animation = new AlphaAnimation(0.0f, 1.0f);
        animation.setDuration(600);

        LayoutAnimationController controller = new LayoutAnimationController(animation);
        mOutputField.setLayoutAnimation(controller);
        mDismiss = (Button) findViewById(R.id.confirm_cancel);
        findViewById(R.id.confirm_update).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    tv.setText(R.string.install_ota_output_start);
                    mUpdateUtils.registerBattery();

                    if (mPref != null) {
                        if ( mPref.getScriptAsk() ) {
                            mPref.createAmlScript(mPackagePath, true, true);
                        }
                        mPref.write2File();
                    }
                    mDismiss.setEnabled(false);
                    TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                                null);
                    tv.setText(R.string.install_prepare);
                    mOutputField.addView(tv);
                    new Thread(new Runnable() {
                            @Override
                            public void run() {
                                mPref.copyBKFile();
                                mUpdateUtils.setDeleteSource(isDelUpdate);
                                mUpdateUtils.upgrade(new File(mPackagePath),
                                    InstallPackage.this, mUpdateMode);
                            }
                        }).start();
                }
            });
    }

    public void setParamter(int updateMode) {
        mUpdateMode = updateMode;
    }

    public void setDelParam(boolean wipe) {
        isDelUpdate = wipe;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!mDismiss.isEnabled() && (keyCode == KeyEvent.KEYCODE_BACK)) {
            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onProgress(final int progress) {
        mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (progress == 0) {
                        TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                                null);
                        tv.setText(R.string.install_ota_output_checking);
                        mOutputField.addView(tv);
                    } else if (progress == 100) {
                        TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                                null);
                        tv.setText(R.string.install_ota_output_check_ok);
                        mOutputField.addView(tv);
                    }

                    mProgressBar.setProgress(progress / 2);
                }
            });
    }

    @Override
    public void onVerifyFailed(int errorCode, Object object) {
        mHandler.post(new Runnable() {
                @Override
                public void run() {
                    TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                            null);
                    tv.setText(R.string.install_ota_output_check_error);
                    mOutputField.addView(tv);
                    mDismiss.setEnabled(true);
                    mUpdateUtils.unregistBattery();
                }
            });
    }

    @Override
    public void onCopyProgress(final int progress) {
        mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (progress == 0) {
                        TextView tv1 = (TextView) mInflater.inflate(R.layout.medium_text,
                                null);
                        tv1.setText(R.string.install_ota_output_copying);
                        mOutputField.addView(tv1);
                    } else if (progress == 100) {
                        TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                                null);
                        tv.setText(R.string.install_ota_output_copy_ok);
                        mOutputField.addView(tv);
                        tv = (TextView) mInflater.inflate(R.layout.medium_text,
                                null);
                        tv.setText(R.string.install_ota_output_restart);
                        mOutputField.addView(tv);
                    }

                    mProgressBar.setProgress(50 + (progress / 2));
                }
            });
    }

    @Override
    public void onCopyFailed(int errorCode, Object object) {
        mHandler.post(new Runnable() {
                @Override
                public void run() {
                    TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                            null);
                    tv.setText(R.string.install_ota_output_copy_failed);
                    mOutputField.addView(tv);
                    mDismiss.setEnabled(true);
                    mUpdateUtils.unregistBattery();
                }
            });
    }

    @Override
    public void onStopProgress(int reason) {
        final int failreason = reason;
        mHandler.post(new Runnable() {
                @Override
                public void run() {
                    TextView tv = (TextView) mInflater.inflate(R.layout.medium_text,
                            null);

                    if (failreason == OtaUpgradeUtils.FAIL_REASON_BATTERY) {
                        tv.setText(R.string.stop_low_battery);
                    }

                    mOutputField.addView(tv);
                    mDismiss.setEnabled(true);
                    mUpdateUtils.unregistBattery();
                }
            });
    }

    /**
     * @param b
     */
    public void deleteSource(boolean b) {
    }
}
