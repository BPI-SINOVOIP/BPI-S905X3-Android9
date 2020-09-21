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

package com.droidlogic.tv.soundeffectsettings;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.transition.Scene;
import android.transition.Slide;
import android.transition.Transition;
import android.transition.TransitionManager;
import android.view.Gravity;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.provider.Settings;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.content.ComponentName;
import com.droidlogic.app.tv.AudioEffectManager;


import java.lang.reflect.Method;

public abstract class TvSettingsActivity extends Activity {

    private static final String TAG = "TvSettingsActivity";
    private static final String SETTINGS_FRAGMENT_TAG =
            "com.droidlogic.tv.settings.MainSettings.SETTINGS_FRAGMENT";
    public static final String INTENT_ACTION_FINISH_FRAGMENT = "action.finish.droidsettingsmodefragment";
    public static final int MODE_LAUNCHER = 0;
    public static final int MODE_LIVE_TV = 1;
    private int mStartMode = MODE_LAUNCHER;
    private SoundParameterSettingManager mSoundParameterSettingManager = null;
    private OptionParameterManager mOptionParameterManager = null;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        init(this);
        if (savedInstanceState == null) {
            final ViewGroup root = (ViewGroup) findViewById(android.R.id.content);
            root.getViewTreeObserver().addOnPreDrawListener(
                    new ViewTreeObserver.OnPreDrawListener() {
                        @Override
                        public boolean onPreDraw() {
                            root.getViewTreeObserver().removeOnPreDrawListener(this);
                            final Scene scene = new Scene(root);
                            scene.setEnterAction(new Runnable() {
                                @Override
                                public void run() {
                                    final Fragment fragment = createSettingsFragment();
                                    if (fragment != null) {
                                        getFragmentManager().beginTransaction()
                                                .add(android.R.id.content, fragment,
                                                        SETTINGS_FRAGMENT_TAG)
                                                .commitNow();
                                    }
                                }
                            });

                            final Slide slide = new Slide(Gravity.END);
                            /*slide.setSlideFraction(
                                    getResources().getDimension(R.dimen.lb_settings_pane_width)
                                            / root.getWidth());*/
                            TransitionManager.go(scene, slide);

                            // Skip the current draw, there's nothing in it
                            return false;
                        }
                    });
        }
        mStartMode = getIntent().getIntExtra("from_live_tv", MODE_LAUNCHER);
        Log.d(TAG, "mStartMode : " + mStartMode);
        if (mStartMode == MODE_LIVE_TV) {
            startShowActivityTimer();
        }
    }

    public BroadcastReceiver mMenuTimeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "intent = " + intent);
            if (intent.getAction().equals(INTENT_ACTION_FINISH_FRAGMENT)) {
                startShowActivityTimer();
            }
        }
    };

    private void init(Context context) {
        mSoundParameterSettingManager = new SoundParameterSettingManager(context);
        mOptionParameterManager = new OptionParameterManager(context);
        getAudioEffectManager();
    }


    public AudioEffectManager getAudioEffectManager() {
        return AudioEffectManager.getInstance(getApplicationContext());
    }

    public SoundParameterSettingManager getSoundParameterSettingManager() {
        return mSoundParameterSettingManager;
    }

    public OptionParameterManager getOptionParameterManager() {
        return mOptionParameterManager;
    }

    public void registerMenuTimeReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(INTENT_ACTION_FINISH_FRAGMENT);
        registerReceiver(mMenuTimeReceiver, intentFilter);
    }

    public void unregisterMenuTimeReceiver() {
        unregisterReceiver(mMenuTimeReceiver);
    }

    public void startShowActivityTimer () {
        handler.removeMessages(0);

        int seconds = Settings.System.getInt(getContentResolver(), OptionParameterManager.KEY_MENU_TIME, OptionParameterManager.DEFUALT_MENU_TIME);
        if (seconds == 1) {
            seconds = 15;
        } else if (seconds == 2) {
            seconds = 30;
        } else if (seconds == 3) {
            seconds = 60;
        } else if (seconds == 4) {
            seconds = 120;
        } else if (seconds == 5) {
            seconds = 240;
        } else {
            seconds = 0;
        }
        if (seconds > 0) {
            handler.sendEmptyMessageDelayed(0, seconds * 1000);
        } else {
            handler.removeMessages(0);
        }
    }

    Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            finish();
        }
    };

    @Override
    public void onResume() {
        registerMenuTimeReceiver();
        if (mStartMode == MODE_LIVE_TV) {
            startShowActivityTimer();
        }
        super.onResume();
    }

    @Override
    public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_BACK:
                    if (mStartMode == MODE_LIVE_TV) {
                        Log.d(TAG, "dispatchKeyEvent");
                        startShowActivityTimer();
                    }
                    break;
                default:
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterMenuTimeReceiver();
        Log.d(TAG, "onDestroy");
    }

    @Override
    public void finish() {
        final Fragment fragment = getFragmentManager().findFragmentByTag(SETTINGS_FRAGMENT_TAG);
        boolean isresumed = true;
        Method isResumedMethod = null;
        try {
            isResumedMethod = Activity.class.getMethod("isResumed()");
            isresumed = (boolean)isResumedMethod.invoke(TvSettingsActivity.this);
        }catch (Exception ex){
        }
        if (isresumed && fragment != null) {
            final ViewGroup root = (ViewGroup) findViewById(android.R.id.content);
            final Scene scene = new Scene(root);
            scene.setEnterAction(new Runnable() {
                @Override
                public void run() {
                    getFragmentManager().beginTransaction()
                            .remove(fragment)
                            .commitNow();
                }
            });
            final Slide slide = new Slide(Gravity.END);
           /* slide.setSlideFraction(
                    getResources().getDimension(R.dimen.lb_settings_pane_width) / root.getWidth());*/
            slide.addListener(new Transition.TransitionListener() {
                @Override
                public void onTransitionStart(Transition transition) {
                    getWindow().setDimAmount(0);
                }

                @Override
                public void onTransitionEnd(Transition transition) {
                    transition.removeListener(this);
                    TvSettingsActivity.super.finish();
                }

                @Override
                public void onTransitionCancel(Transition transition) {
                }

                @Override
                public void onTransitionPause(Transition transition) {
                }

                @Override
                public void onTransitionResume(Transition transition) {
                }
            });
            TransitionManager.go(scene, slide);
        } else {
            super.finish();
        }
    }

    protected abstract Fragment createSettingsFragment();
}
