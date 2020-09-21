/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.app.Activity;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.provider.Settings;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.view.View;
import android.view.KeyEvent;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.Window;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.view.inputmethod.InputMethodManager;
import android.util.Log;
import android.text.TextUtils;
import android.media.AudioManager;
import android.media.tv.TvInputInfo;
import java.lang.reflect.Method;

import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.tvinput.R;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvControlDataManager;

public class TvSettingsActivity extends Activity implements OnClickListener, OnFocusChangeListener {
    private static final String TAG = "TvSettingsActivity";

    private ContentFragment fragmentImage;
    private ContentFragment fragmentSound;
    private ContentFragment fragmentChannel;
    private ContentFragment fragmentSettings;
    public RelativeLayout mOptionLayout = null;

    private ContentFragment currentFragment;
    private SettingsManager mSettingsManager;
    private OptionUiManager mOptionUiManager;
    private PowerManager mPowerManager;

    private ImageButton tabPicture;
    private ImageButton tabSound;
    private ImageButton tabChannel;
    private ImageButton tabSettings;
    private TvControlManager mTvControlManager;
    private TvControlDataManager mTvControlDataManager = null;
    private AudioManager mAudioManager = null;
    private boolean isFinished = false;

    /*password to enter factory menu*/
    private int factoryPasswordFirstKey = 8;
    private int factoryPasswordSecondKey = 8;
    private int factoryPasswordThirdKey = 9;
    private int factoryPasswordFourthKey = 3;
    private int mFactoryShow = 0;

    public ScanEdit mScanEdit;
    public ManualScanEdit mManualScanEdit;

    private TvInputInfo mTvInputInfo;
    private Boolean isNeedStopTv = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.layout_main);
        isFinished = false;

        mSettingsManager = new SettingsManager(this, getIntent());
        mTvControlManager = TvControlManager.getInstance();
        mTvControlDataManager = TvControlDataManager.getInstance(this);
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        mPowerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);

        tabPicture = (ImageButton) findViewById(R.id.button_picture);
        tabSound = (ImageButton) findViewById(R.id.button_sound);
        tabChannel = (ImageButton) findViewById(R.id.button_channel);
        tabSettings = (ImageButton) findViewById(R.id.button_settings);
        tabPicture.setOnClickListener(this);
        tabPicture.setOnFocusChangeListener(this);
        tabSound.setOnClickListener(this);
        tabSound.setOnFocusChangeListener(this);
        tabChannel.setOnClickListener(this);
        tabChannel.setOnFocusChangeListener(this);
        tabSettings.setOnClickListener(this);
        tabSettings.setOnFocusChangeListener(this);

        tabPicture.setNextFocusUpId(tabPicture.getId());
        tabPicture.setNextFocusDownId(tabSound.getId());
        tabSound.setNextFocusUpId(tabPicture.getId());
        tabChannel.setNextFocusUpId(tabSound.getId());
        tabChannel.setNextFocusDownId(tabSettings.getId());
        if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI ||
            mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_AV) {
            tabSound.setNextFocusDownId(tabSettings.getId());
            tabSettings.setNextFocusUpId(tabSound.getId());
            tabChannel.setVisibility(View.GONE);
            findViewById(R.id.title_channel).setVisibility(View.GONE);
        } else {
            tabSound.setNextFocusDownId(tabChannel.getId());
            tabSettings.setNextFocusUpId(tabChannel.getId());
        }
        tabSettings.setNextFocusDownId(tabSettings.getId());

        if (savedInstanceState == null)
            setDefaultFragment();

        mOptionUiManager = new OptionUiManager(this);
        startShowActivityTimer();

        mSettingsManager.startTvPlayAndSetSourceInput();
    }

    private void setDefaultFragment() {
        FragmentManager fm = getFragmentManager();
        FragmentTransaction transaction = fm.beginTransaction();
        currentFragment = new ContentFragment(R.xml.list_picture, tabPicture);
        transaction.replace(R.id.settings_list, currentFragment);
        transaction.commit();
    }

    @Override
    public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if (mOptionUiManager.isSearching())
                        return true;
                    startShowActivityTimer();
                    break;
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    if (mOptionUiManager.isSearching())
                        break;
                    startShowActivityTimer();
                    break;
                case KeyEvent.KEYCODE_0:
                    enterFactoryCount(0);
                    break;
                case KeyEvent.KEYCODE_1:
                    enterFactoryCount(1);
                    break;
                case KeyEvent.KEYCODE_2:
                    enterFactoryCount(2);
                    break;
                case KeyEvent.KEYCODE_3:
                    enterFactoryCount(3);
                    break;
                case KeyEvent.KEYCODE_4:
                    enterFactoryCount(4);
                    break;
                case KeyEvent.KEYCODE_5:
                    enterFactoryCount(5);
                    break;
                case KeyEvent.KEYCODE_6:
                    enterFactoryCount(6);
                    break;
                case KeyEvent.KEYCODE_7:
                    enterFactoryCount(7);
                    break;
                case KeyEvent.KEYCODE_8:
                    enterFactoryCount(8);
                    break;
                case KeyEvent.KEYCODE_9:
                    enterFactoryCount(9);
                    break;
                default:
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }

    private boolean isMasterMute() {
        try {
              Class<?> cls = Class.forName("android.media.AudioManager");
              Method method = cls.getMethod("isMasterMute");
              Object objbool = method.invoke(mAudioManager);
              return Boolean.parseBoolean(objbool.toString());
          } catch (Exception e) {
                  // TODO Auto-generated catch block
              e.printStackTrace();
          }
          return false;
    }

    private void setMasterMute(boolean bval, int val) {
        try {
            Class<?> cls = Class.forName("android.media.AudioManager");
            Method method = cls.getMethod("setMasterMute", boolean.class, int.class);
            method.invoke(mAudioManager, bval, val);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        //Log.d(TAG, "==== focus =" + getCurrentFocus() + ", keycode =" + keyCode);
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                if (mOptionUiManager.isSearching()) {
                    mTvControlManager.DtvStopScan();
                } else {
                    isNeedStopTv = false;
                    finish();
                }
                return true;
            case KeyEvent.KEYCODE_MENU:
                if (mOptionUiManager.isSearching()) {
                    mTvControlManager.DtvStopScan();
                    return true;
                }
                isNeedStopTv = false;
                finish();
                break;
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                if (isMasterMute()) {
                    setMasterMute(false, AudioManager.FLAG_PLAY_SOUND);
                    mSettingsManager.sendBroadcastToTvapp("unmute");
                }
                break;
            case KeyEvent.KEYCODE_VOLUME_MUTE:
                if (isMasterMute()) {
                    setMasterMute(false, AudioManager.FLAG_PLAY_SOUND);
                    mSettingsManager.sendBroadcastToTvapp("unmute");
                } else {
                    setMasterMute(true, AudioManager.FLAG_PLAY_SOUND);
                    mSettingsManager.sendBroadcastToTvapp("mute");
                }
                return true;
            default:
                break;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onClick(View v) {
        /* FragmentManager fm = getFragmentManager();
         FragmentTransaction transaction = fm.beginTransaction();

         switch (v.getId()) {
             case R.id.button_picture:
                 if (fragmentImage == null) {
                     fragmentImage = new ContentFragment(R.xml.list_picture);
                 }
                 transaction.replace(R.id.settings_list, fragmentImage);
                 break;
             case R.id.button_sound:
                 if (fragmentSound == null) {
                     fragmentSound = new ContentFragment(R.xml.list_sound);
                 }
                 transaction.replace(R.id.settings_list, fragmentSound);
                 break;
             case R.id.button_channel:
                 if (fragmentChannel== null) {
                     fragmentChannel= new ContentFragment(R.xml.list_channel);
                 }
                 transaction.replace(R.id.settings_list, fragmentChannel);
                 break;
             case R.id.button_settings:
                 if (fragmentSettings== null) {
                     fragmentSettings= new ContentFragment(R.xml.list_settings);
                 }
                 transaction.replace(R.id.settings_list, fragmentSettings);
                 break;
         }
         // transaction.addToBackStack();
         transaction.commit();*/
    }

    @Override
    public void onFocusChange (View v, boolean hasFocus) {
        if (hasFocus) {
            if (isFinished)
                return;

            RelativeLayout main_view = (RelativeLayout)findViewById(R.id.main);
            if (mOptionLayout != null)
                main_view.removeView(mOptionLayout);

            FragmentManager fm = getFragmentManager();
            FragmentTransaction transaction = fm.beginTransaction();

            switch (v.getId()) {
                case R.id.button_picture:
                    if (fragmentImage == null) {
                        fragmentImage = new ContentFragment(R.xml.list_picture, v);
                    }
                    currentFragment = fragmentImage;
                    break;
                case R.id.button_sound:
                    if (fragmentSound == null) {
                        fragmentSound = new ContentFragment(R.xml.list_sound, v);
                    }
                    currentFragment = fragmentSound;
                    break;
                case R.id.button_channel:
                    if (fragmentChannel == null) {
                        if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV)
                            fragmentChannel = new ContentFragment(R.xml.list_channel_dtv, v);
                       else  if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV)
                            fragmentChannel = new ContentFragment(R.xml.list_channel_atv, v);
                   }
                    currentFragment = fragmentChannel;
                    break;
                case R.id.button_settings:
                    if (fragmentSettings == null) {
                        fragmentSettings = new ContentFragment(R.xml.list_settings, v);
                    }
                    currentFragment = fragmentSettings;
                    break;
                default:
                    if (fragmentImage == null) {
                        fragmentImage = new ContentFragment(R.xml.list_picture, v);
                    }
                    currentFragment = fragmentImage;
                    break;
            }

            transaction.replace(R.id.settings_list, currentFragment);
            transaction.commit();
            // transaction.addToBackStack();
        }
    }

    public ContentFragment getCurrentFragment () {
        return currentFragment;
    }

    public SettingsManager getSettingsManager () {
        return mSettingsManager;
    }

    public OptionUiManager getOptionUiManager () {
        return mOptionUiManager;
    }

    @Override
    public void finish() {
        isFinished = true;

        if (!mPowerManager.isScreenOn()) {
            Log.d(TAG, "TV is going to sleep, stop tv");
            return;
        }

        if (mSettingsManager != null) {
            setResult(mSettingsManager.getActivityResult());
        }
        super.finish();
    }

    @Override
    public void onResume() {
        if (isFinished) {
            resume();
            isFinished = false;
        }

        super.onResume();
        Log.d(TAG, "onResume");
        IntentFilter filter = new IntentFilter();
        filter.addAction(DroidLogicTvUtils.ACTION_CHANNEL_CHANGED);
        filter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(mReceiver, filter);

    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
        unregisterReceiver(mReceiver);

        if (mOptionUiManager.isSearching()) {
            mTvControlManager.DtvStopScan();

            /*if (!mPowerManager.isScreenOn()) {
                mTvControlManager.StopTv();
            }*/
        }
        if (isNeedStopTv) {
            mTvControlManager.StopTv();
            Log.d(TAG, "stoptv");
        }
        isNeedStopTv = true;

        if (!isFinishing()) {
            finish();
        }
    }

    @Override
    public void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();
        release();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        super.onDestroy();
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        Log.d(TAG, "finalized");
    }

    public void startShowActivityTimer () {
        handler.removeMessages(0);

        int seconds = mTvControlDataManager.getInt(getContentResolver(), SettingsManager.KEY_MENU_TIME, SettingsManager.DEFUALT_MENU_TIME);
        if (seconds == 0) {
            seconds = 10;
        } else if (seconds == 1) {
            seconds = 20;
        } else if (seconds == 2) {
            seconds = 40;
        } else if (seconds == 3) {
            seconds = 60;
        }
        handler.sendEmptyMessageDelayed(0, seconds * 1000);
    }

    Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
            if (!mOptionUiManager.isSearching() && !imm. isAcceptingText()) {
                finish();
                isNeedStopTv = false;
            } else  {
                int seconds = mTvControlDataManager.getInt(getContentResolver(), SettingsManager.KEY_MENU_TIME, SettingsManager.DEFUALT_MENU_TIME);
                if (seconds == 0) {
                    seconds = 10;
                } else if (seconds == 1) {
                    seconds = 20;
                } else if (seconds == 2) {
                    seconds = 40;
                } else if (seconds == 3) {
                    seconds = 60;
                }
                sendEmptyMessageDelayed(0, seconds * 1000);
            }
        }
    };

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(DroidLogicTvUtils.ACTION_CHANNEL_CHANGED)) {
                mSettingsManager.setCurrentChannelData(intent);
                    mOptionUiManager.init(mSettingsManager);
                currentFragment.refreshList();
            } else if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                String reason = intent.getStringExtra("reason");
                if (TextUtils.equals(reason, "homekey")) {
                    finish();
                }
            }
        }
    };

    private void resume() {
        mSettingsManager = new SettingsManager(this, getIntent());
        mTvControlManager = TvControlManager.getInstance();
        mOptionUiManager = new OptionUiManager(this);

        startShowActivityTimer();
    }

    private void release() {
        handler.removeCallbacksAndMessages(null);
        mSettingsManager = null;
        mOptionUiManager = null;
        mTvControlManager = null;

        RelativeLayout main_view = (RelativeLayout)findViewById(R.id.main);
        if (mOptionLayout != null) {
            main_view.removeView(mOptionLayout);
        }
    }

    /*add enter factory menu*/
    private void enterFactoryCount(int keyValue) {
        if (mFactoryShow == 0 && keyValue == factoryPasswordFirstKey) {
            mFactoryShow = 1;
            return;
        }
        if (mFactoryShow == 1 && keyValue == factoryPasswordSecondKey) {
            mFactoryShow = 2;
            return;
        }
        if (mFactoryShow == 2 && keyValue == factoryPasswordThirdKey) {
            mFactoryShow = 3;
            return;
        }
        if (mFactoryShow == 3 && keyValue == factoryPasswordFourthKey) {
            mFactoryShow = 0;
            this.finish();
            Intent factory = new Intent("droidlogic.intent.action.FactoryMainActivity");
            startActivity(factory);
            return;
        }
        mFactoryShow = 0;
    }
}
