/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.shortcut;

import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.media.tv.TvContract.Programs;
import com.droidlogic.app.DroidLogicKeyEvent;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.Program;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvTime;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.tvinput.settings.SettingsManager;
import com.droidlogic.tvinput.shortcut.GuideListView.ListItemSelectedListener;

import android.provider.Settings;
import android.app.Activity;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.database.ContentObserver;
//import android.database.IContentObserver;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.util.ArrayMap;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import com.droidlogic.tvinput.R;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.TimeZone;
import java.text.SimpleDateFormat;

public class ShortCutActivity extends Activity implements ListItemSelectedListener, OnItemClickListener {
    private static final String TAG = "ShortCutActivity";

    private static final int MSG_FINISH = 0;
    private static final int MSG_UPDATE_DATE = 1;
    private static final int MSG_UPDATE_PROGRAM = 2;
    private static final int MSG_LOAD_DATE = 3;
    private static final int MSG_LOAD_PROGRAM = 4;
    private static final int MSG_UPDATE_CHANNELS = 5;
    private static final int MSG_LOAD_CHANNELS = 6;

    private static final int TOAST_SHOW_TIME = 3000;

    private static final long DAY_TO_MS = 86400000;
    private static final long PROGRAM_INTERVAL = 60000;

    private SettingsManager mSettingsManager;
    private TvDataBaseManager mTvDataBaseManager;
    private Resources mResources;
    private View viewToast = null;
    private Toast toast = null;
    private Toast guide_toast = null;

    private GuideListView lv_channel;
    private GuideListView lv_date;
    private GuideListView lv_program;
    private TextView tx_date;
    private TextView tx_program_description;
    private ArrayList<ChannelInfo> channelInfoList;
    private ArrayList<ArrayMap<String, Object>> list_channels  = new ArrayList<ArrayMap<String, Object>>();
    private ArrayList<ArrayMap<String, Object>> list_date = new ArrayList<ArrayMap<String, Object>>();
    private ArrayList<ArrayMap<String, Object>> list_program = new ArrayList<ArrayMap<String, Object>>();
    private SimpleAdapter channelsAdapter;
    private ProgramObserver mProgramObserver;
    private ChannelObserver mChannelObserver;
    private int currentChannelIndex = -1;
    private int currentDateIndex = -1;
    private int currentProgramIndex = -1;
    private TvTime mTvTime = null;
    private TextView toast_title;
    private TextView toast_status;
    private LinearLayout mToast;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.layout_shortcut_key);
        mToast = (LinearLayout) findViewById(R.id.mToast);
        toast_title = (TextView) findViewById(R.id.toast_title);
        toast_status = (TextView) findViewById(R.id.toast_status);
        mSettingsManager = new SettingsManager(this, getIntent());
        mTvDataBaseManager = new TvDataBaseManager(this);
        mResources = getResources();

        int mode = getIntent().getIntExtra(DroidLogicTvUtils.EXTRA_KEY_CODE, 0);
        setShortcutMode(mode);
    }

    @Override
    protected void onStart() {
        Log.d(TAG, "------onStart");
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_TIME_TICK);
        filter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        filter.addAction(Intent.ACTION_TIME_CHANGED);
        registerReceiver(mReceiver, filter);
        super.onStart();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "------onStop");
        unregisterReceiver(mReceiver);
        if (mProgramObserver != null) {
            getContentResolver().unregisterContentObserver(mProgramObserver);
            mProgramObserver = null;
        }
        if (mChannelObserver != null) {
            getContentResolver().unregisterContentObserver(mChannelObserver);
            mChannelObserver = null;
        }
        super.onStop();
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE:
                setShortcutMode(keyCode);
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE:
                setShortcutMode(keyCode);
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE:
                setShortcutMode(keyCode);
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP:
                setShortcutMode(keyCode);
                break;
            case DroidLogicKeyEvent.KEYCODE_GUIDE:
                if (mTvTime != null) {
                    finish();
                }
                break;
            default:
                break;
        }
        return super.onKeyDown(keyCode, event);
    }

    private void setShortcutMode (int mode) {
        switch (mode) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE:
                String display_mode = mSettingsManager.getAspectRatioStatus();
                if (display_mode.equals(mResources.getString(R.string.auto))) {
                    mSettingsManager.setAspectRatio(SettingsManager.STATUS_4_TO_3);
                } else if (display_mode.equals(mResources.getString(R.string.four2three))) {
                    mSettingsManager.setAspectRatio(SettingsManager.STATUS_PANORAMA);
                    mSettingsManager.setAspectRatio(SettingsManager.STATUS_FULL_SCREEN);
                } else if (display_mode.equals(mResources.getString(R.string.panorama))) {
                    mSettingsManager.setAspectRatio(SettingsManager.STATUS_FULL_SCREEN);
                } else if (display_mode.equals(mResources.getString(R.string.full_screen))) {
                    mSettingsManager.setAspectRatio(SettingsManager.STATUS_AUTO);
                }
                showCustomToast(mode);
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_3DMODE:
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE:
                String picture_mode = mSettingsManager.getPictureModeStatus();
                if (picture_mode.equals(mResources.getString(R.string.standard))) {
                    mSettingsManager.setPictureMode(SettingsManager.STATUS_VIVID);
                } else if (picture_mode.equals(mResources.getString(R.string.vivid))) {
                    mSettingsManager.setPictureMode(SettingsManager.STATUS_SOFT);
                } else if (picture_mode.equals(mResources.getString(R.string.soft))) {
                    mSettingsManager.setPictureMode(SettingsManager.STATUS_MONITOR);
                } else if (picture_mode.equals(mResources.getString(R.string.monitor))) {
                    mSettingsManager.setPictureMode(SettingsManager.STATUS_USER);
                } else if (picture_mode.equals(mResources.getString(R.string.user))) {
                    mSettingsManager.setPictureMode(SettingsManager.STATUS_STANDARD);
                }
                showCustomToast(mode);
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE:
                String sound_mode = mSettingsManager.getSoundModeStatus();
                if (sound_mode.equals(mResources.getString(R.string.standard))) {
                    mSettingsManager.setSoundMode(SettingsManager.STATUS_MUSIC);
                } else if (sound_mode.equals(mResources.getString(R.string.music))) {
                    mSettingsManager.setSoundMode(SettingsManager.STATUS_NEWS);
                } else if (sound_mode.equals(mResources.getString(R.string.news))) {
                    mSettingsManager.setSoundMode(SettingsManager.STATUS_MOVIE);
                } else if (sound_mode.equals(mResources.getString(R.string.movie))) {
                    mSettingsManager.setSoundMode(SettingsManager.STATUS_USER);
                } else if (sound_mode.equals(mResources.getString(R.string.user))) {
                    mSettingsManager.setSoundMode(SettingsManager.STATUS_STANDARD);
                }
                showCustomToast(mode);
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP:
                String sleep_time = mSettingsManager.getSleepTimerStatus();
                if (sleep_time.equals(mResources.getString(R.string.off))) {
                    mSettingsManager.setSleepTimer(15);
                } else if (sleep_time.equals(mResources.getString(R.string.time_15min))) {
                    mSettingsManager.setSleepTimer(30);
                } else if (sleep_time.equals(mResources.getString(R.string.time_30min))) {
                    mSettingsManager.setSleepTimer(45);
                } else if (sleep_time.equals(mResources.getString(R.string.time_45min))) {
                    mSettingsManager.setSleepTimer(60);
                } else if (sleep_time.equals(mResources.getString(R.string.time_60min))) {
                    mSettingsManager.setSleepTimer(90);
                } else if (sleep_time.equals(mResources.getString(R.string.time_90min))) {
                    mSettingsManager.setSleepTimer(120);
                } else if (sleep_time.equals(mResources.getString(R.string.time_120min))) {
                    mSettingsManager.setSleepTimer(0);
                }
                showCustomToast(mode);
                break;
            case DroidLogicKeyEvent.KEYCODE_MEDIA_AUDIO_CONTROL:

                break;
            case DroidLogicKeyEvent.KEYCODE_GUIDE:
                setContentView(R.layout.layout_shortcut_guide);
                setGuideView();
                break;
            default:
                break;
        }
    }

    private void showCustomToast(int mode) {
       /* if (viewToast == null) {
            LayoutInflater inflater = getLayoutInflater();
            viewToast = inflater.inflate(R.layout.layout_shortcut_key, null);
        }*/

        String str_title = null;
        String str_status = null;
        switch (mode) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE:
                mToast.setBackgroundResource(R.drawable.shortcut_ratio);
                str_title = mResources.getString(R.string.aspect_ratio);
                str_status = mSettingsManager.getAspectRatioStatus();
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_3DMODE:
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE:
                mToast.setBackgroundResource(R.drawable.shortcut_picture);
                str_title = mResources.getString(R.string.picture_mode);
                str_status = mSettingsManager.getPictureModeStatus();
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE:
                mToast.setBackgroundResource(R.drawable.shortcut_sound);
                str_title = mResources.getString(R.string.sound_mode);
                str_status = mSettingsManager.getSoundModeStatus();
                break;
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP:
                mToast.setBackgroundResource(R.drawable.shortcut_sleep);
                str_title = mResources.getString(R.string.sleep_timer);
                str_status = mSettingsManager.getSleepTimerStatus();
                break;
            case DroidLogicKeyEvent.KEYCODE_MEDIA_AUDIO_CONTROL:
                break;
            case DroidLogicKeyEvent.KEYCODE_GUIDE:
                break;
            default:
                break;
        }

        toast_title.setText(str_title);
        toast_status.setText(str_status);
        mToast.setVisibility(View.VISIBLE);
       /* if (toast == null) {
            toast = new Toast(this);
            toast.setDuration(TOAST_SHOW_TIME);
            toast.setGravity(Gravity.CENTER_VERTICAL, 400, 300);
            toast.setView(viewToast);
        }
        toast.show();
        */
        startShowActivityTimer();

    }

    public void startShowActivityTimer () {
        handler.removeMessages(MSG_FINISH);
        handler.sendEmptyMessageDelayed(MSG_FINISH, TOAST_SHOW_TIME);
    }

    Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FINISH:
                    mToast.setVisibility(View.GONE);
                    finish();
                    break;
                case MSG_UPDATE_CHANNELS:
                    showChannelList();
                    break;
                case MSG_UPDATE_DATE:
                    showDateList();
                    break;
                case MSG_UPDATE_PROGRAM:
                    showProgramList();
                    break;
                case MSG_LOAD_CHANNELS:
                    new Thread(getChannelRunnable).start();
                    break;
                case MSG_LOAD_DATE:
                    if (msg.arg1 != -1) {
                        currentDateIndex = -1;
                        currentProgramIndex = -1;
                    }
                    Log.d(TAG, "current Date:" + currentDateIndex);
                    new Thread(getDateRunnable).start();
                    break;
                case MSG_LOAD_PROGRAM:
                    Log.d(TAG, "current Program:" + currentProgramIndex);
                    new Thread(getProgramRunnable).start();
                    break;
                default:
                    break;
            }
        }
    };

    BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_TIME_TICK)) {
                if (tx_date != null) {
                    String[] dateAndTime = getDateAndTime(mTvTime.getTime());
                    String currentTime = dateAndTime[0] + "." + dateAndTime[1] + "." + dateAndTime[2] + "   " + dateAndTime[3] + ":" + dateAndTime[4];

                    tx_date.setText(currentTime);
                } else if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                    String reason = intent.getStringExtra("reason");
                    if (TextUtils.equals(reason, "homekey")) {
                        finish();
                    }
                } else if (action.equals(Intent.ACTION_TIME_CHANGED)) {
                    Log.d(TAG, "SysTime changed.");
                    loadDateTime();
                }
            }
        }
    };

    private Runnable getChannelRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                handler.removeMessages(MSG_LOAD_CHANNELS);
                loadChannelList();
            } catch (RuntimeException e) {
                e.printStackTrace();
            }
        }
    };

    private Runnable getDateRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                handler.removeMessages(MSG_LOAD_DATE);
                loadDateList();
            } catch (RuntimeException e) {
                e.printStackTrace();
            }
        }
    };

    private Runnable getProgramRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                handler.removeMessages(MSG_LOAD_PROGRAM);
                loadProgramList();
            } catch (RuntimeException e) {
                e.printStackTrace();
            }
        }
    };

    private void setGuideView() {
        mTvTime = new TvTime(this);

        loadDateTime();

        tx_program_description = (TextView)findViewById(R.id.guide_details_content);

        lv_channel = (GuideListView)findViewById(R.id.list_guide_channel);
        lv_date = (GuideListView)findViewById(R.id.list_guide_week);
        lv_program = (GuideListView)findViewById(R.id.list_guide_programs);

        lv_channel.setListItemSelectedListener(this);
        lv_channel.setOnItemClickListener(this);

        lv_date.setListItemSelectedListener(this);

        lv_program.setListItemSelectedListener(this);
        lv_program.setOnItemClickListener(this);

        if (mProgramObserver == null)
            mProgramObserver = new ProgramObserver();
        if (mChannelObserver == null)
            mChannelObserver = new ChannelObserver();

        getContentResolver().registerContentObserver(TvContract.Programs.CONTENT_URI, true, mProgramObserver);
        getContentResolver().registerContentObserver(TvContract.Channels.CONTENT_URI, true, mChannelObserver);

        handler.sendEmptyMessage(MSG_LOAD_CHANNELS);
    }

    private void loadDateTime() {
        tx_date = (TextView)findViewById(R.id.guide_date);
        String[] dateAndTime = getDateAndTime(mTvTime.getTime());
        tx_date.setText(dateAndTime[0] + "." + dateAndTime[1] + "." + dateAndTime[2] + "   " + dateAndTime[3] + ":" + dateAndTime[4]);
    }

    public ArrayList<ArrayMap<String, Object>> getChannelList (ArrayList<ChannelInfo> channelInfoList) {
        ArrayList<ArrayMap<String, Object>> list =  new ArrayList<ArrayMap<String, Object>>();
        if (channelInfoList.size() > 0) {
            for (int i = 0 ; i < channelInfoList.size(); i++) {
                ChannelInfo info = channelInfoList.get(i);
                if (info != null) {
                    ArrayMap<String, Object> item = new ArrayMap<String, Object>();

                    item.put(GuideListView.ITEM_1, info.getNumber() + "  " + info.getDisplayNameLocal());
                    item.put(GuideListView.ITEM_2, info.getNumber());
                    if (ChannelInfo.isRadioChannel(info)) {
                        item.put(GuideListView.ITEM_3, true);
                    } else {
                        item.put(GuideListView.ITEM_3, false);
                    }
                    list.add(item);
                    if (mSettingsManager.getCurrentChannelNumber() == info.getNumber())
                        currentChannelIndex = i;
                }
            }
        }
        return list;
    }

    private void loadChannelList() {
        Log.d(TAG, "load Channels");

        list_channels.clear();

        channelInfoList = mTvDataBaseManager.getChannelList(mSettingsManager.getInputId(), Channels.SERVICE_TYPE_AUDIO_VIDEO, true);
        channelInfoList.addAll(mTvDataBaseManager.getChannelList(mSettingsManager.getInputId(), Channels.SERVICE_TYPE_AUDIO, true));

        list_channels = getChannelList(channelInfoList);

        handler.sendEmptyMessage(MSG_UPDATE_CHANNELS);
    }

    private void showChannelList() {
        Log.d(TAG, "show Channels");
        handler.removeMessages(MSG_UPDATE_CHANNELS);

        ArrayList<ArrayMap<String, Object>> list = new ArrayList<ArrayMap<String, Object>>();
        list.addAll(list_channels);

        channelsAdapter = new SimpleAdapter(this, list,
                                            R.layout.layout_guide_single_text,
                                            new String[] {GuideListView.ITEM_1}, new int[] {R.id.text_name});
        lv_channel.setAdapter(channelsAdapter);

        currentChannelIndex = (currentChannelIndex != -1 ? currentChannelIndex : 0);
        lv_channel.setSelection(currentChannelIndex);
    }

    private void loadDateList() {
        Log.d(TAG, "load Date");

        list_date.clear();

        int saveChannelIndex = currentChannelIndex;
        ChannelInfo currentChannel = channelInfoList.get(saveChannelIndex);
        List<Program> channel_programs = mTvDataBaseManager.getPrograms(TvContract.buildProgramsUriForChannel(currentChannel.getId()));
        if (channel_programs.size() > 0) {
            long firstProgramTime = channel_programs.get(0).getStartTimeUtcMillis();
            long lastProgramTime = channel_programs.get(channel_programs.size() - 1).getStartTimeUtcMillis();
            int time_offset = TimeZone.getDefault().getOffset(firstProgramTime);

            long tmp_time = (firstProgramTime) - ((firstProgramTime + time_offset) % DAY_TO_MS);
            int count = 0;
            while ((tmp_time <= lastProgramTime) && count < 20) {
                count++;
                if (currentDateIndex == -1) {
                    if (mTvTime.getTime() >= tmp_time && mTvTime.getTime() < tmp_time + DAY_TO_MS)
                        currentDateIndex = count - 1;
                }
                ArrayMap<String, Object> item = new ArrayMap<String, Object>();
                String[] dateAndTime = getDateAndTime(tmp_time);
                item.put(GuideListView.ITEM_1, dateAndTime[1] + "." + dateAndTime[2]);
                item.put(GuideListView.ITEM_2, Long.toString(tmp_time));
                tmp_time = tmp_time + DAY_TO_MS;
                item.put(GuideListView.ITEM_3, Long.toString(tmp_time - 1));
                if (saveChannelIndex != currentChannelIndex) {
                    return;
                }

                /*ignore the days before today*/
                if (tmp_time < mTvTime.getTime())
                    continue;

                list_date.add(item);
            }
        } else {
            ArrayMap<String, Object> item = new ArrayMap<String, Object>();
            item.put(GuideListView.ITEM_1, mResources.getString(R.string.no_program));

            if (saveChannelIndex != currentChannelIndex) {
                return;
            }
            list_date.add(item);
        }
        if (saveChannelIndex == currentChannelIndex)
            handler.sendEmptyMessage(MSG_UPDATE_DATE);
    }

    private void showDateList() {
        Log.d(TAG, "show Date");

        handler.removeMessages(MSG_UPDATE_DATE);
        ArrayList<ArrayMap<String, Object>> list = new ArrayList<ArrayMap<String, Object>>();
        list.addAll(list_date);

        SimpleAdapter dateAdapter = new SimpleAdapter(this, list,
                R.layout.layout_guide_single_text_center,
                new String[] {GuideListView.ITEM_1}, new int[] {R.id.text_name});
        lv_date.setAdapter(dateAdapter);

        currentDateIndex = (currentDateIndex != -1 ? currentDateIndex : 0);
        lv_date.setSelection(currentDateIndex);
    }

    private void loadProgramList() {
        Log.d(TAG, "load Program");

        list_program.clear();

        int saveChannelIndex = currentChannelIndex;
        if (list_date.get(currentDateIndex).get(GuideListView.ITEM_2) != null) {
            long dayStartTime = Long.valueOf(list_date.get(currentDateIndex).get(GuideListView.ITEM_2).toString());
            long dayEndTime = Long.valueOf(list_date.get(currentDateIndex).get(GuideListView.ITEM_3).toString());
            ChannelInfo currentChannel = channelInfoList.get(saveChannelIndex);
            long now = mTvTime.getTime();
            if (now >= dayStartTime && now <= dayEndTime)
                dayStartTime = now;
            List<Program> programs = mTvDataBaseManager.getPrograms(
                        TvContract.buildProgramsUriForChannel(currentChannel.getId(), dayStartTime, dayEndTime));

            for (int i = 0; i < programs.size(); i++) {
                Program program = programs.get(i);
                String[] dateAndTime = getDateAndTime(program.getStartTimeUtcMillis());
                String[] endTime = getDateAndTime(program.getEndTimeUtcMillis());
                String month_and_date = dateAndTime[1] + "." + dateAndTime[2];
                String status = "";

                ArrayMap<String, Object> item_program = new ArrayMap<String, Object>();

                item_program.put(GuideListView.ITEM_1, dateAndTime[3] + ":" + dateAndTime[4]
                                 + "~" + endTime[3] + ":" + endTime[4]);
                item_program.put(GuideListView.ITEM_2, program.getTitle());
                item_program.put(GuideListView.ITEM_3, program.getDescription());
                item_program.put(GuideListView.ITEM_4, Long.toString(program.getId()));

                if (mTvTime.getTime() >= program.getStartTimeUtcMillis() && mTvTime.getTime() <= program.getEndTimeUtcMillis()) {
                    if (currentProgramIndex == -1)
                        currentProgramIndex = i;
                    status = GuideListView.STATUS_PLAYING;
                } else if (program.isAppointed()) {
                    status = GuideListView.STATUS_APPOINTED;
                }
                item_program.put(GuideListView.ITEM_5, status);

                if (saveChannelIndex != currentChannelIndex) {
                    return;
                }
                list_program.add(item_program);
            }
        }
        if (list_program.size() == 0) {
            ArrayMap<String, Object> item = new ArrayMap<String, Object>();
            item.put(GuideListView.ITEM_1, mResources.getString(R.string.no_program));

            if (saveChannelIndex != currentChannelIndex) {
                return;
            }
            list_program.add(item);
        }

        if (saveChannelIndex == currentChannelIndex)
            handler.sendEmptyMessage(MSG_UPDATE_PROGRAM);
    }

    private void showProgramList() {
        Log.d(TAG, "show Program");
        handler.removeMessages(MSG_UPDATE_PROGRAM);

        ArrayList<ArrayMap<String, Object>> list = new ArrayList<ArrayMap<String, Object>>();
        list.addAll(list_program);

        if (lv_program.getAdapter() == null) {
            GuideAdapter programAdapter = new GuideAdapter(this, list);
            lv_program.setAdapter(programAdapter);
            currentProgramIndex = (currentProgramIndex != -1 ? currentProgramIndex : 0);
            lv_program.setSelection(currentProgramIndex);
        } else {
            ((GuideAdapter)lv_program.getAdapter()).refill(list);
        }
    }


    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.list_guide_channel:
                sendSwitchChannelBroadcast(position);
                break;
            case R.id.list_guide_programs:
                if (list_program.size() > position) {
                    Object programId_object = list_program.get(position).get(GuideListView.ITEM_4);
                    if (programId_object != null) {
                        long programId = Long.valueOf(list_program.get(position).get(GuideListView.ITEM_4).toString());
                        Program program = mTvDataBaseManager.getProgram(programId);
                        String appointed_status;

                        if (mTvTime.getTime() < program.getStartTimeUtcMillis()) {
                            if (program.isAppointed()) {
                                program.setIsAppointed(false);
                                ((ImageView)view.findViewById(R.id.img_appointed)).setImageResource(0);
                                appointed_status = mResources.getString(R.string.appointed_cancel);
                                mTvDataBaseManager.updateProgram(program);
                                cancelAppointedProgramAlarm(program);
                            } else {
                                program.setIsAppointed(true);
                                ((ImageView)view.findViewById(R.id.img_appointed)).setImageResource(R.drawable.appointed);
                                appointed_status = mResources.getString(R.string.appointed_success) + setAppointedProgramAlarm(program);
                                mTvDataBaseManager.updateProgram(program);
                            }
                        } else {
                            appointed_status = mResources.getString(R.string.appointed_expired);
                        }
                        showGuideToast(appointed_status);
                    }
                }
        }
    }

    @Override
    public void onListItemSelected(View parent, int position) {
        switch (parent.getId()) {
            case R.id.list_guide_channel:
                lv_date.setAdapter(null);
                lv_program.setAdapter(null);
                currentChannelIndex = position;
                handler.sendEmptyMessage(MSG_LOAD_DATE);
                break;
            case R.id.list_guide_week:
                currentDateIndex = position;
                handler.sendEmptyMessage(MSG_LOAD_PROGRAM);
                break;
            case R.id.list_guide_programs:
                if (position < list_program.size()) {
                    currentProgramIndex = position;
                    Object description = list_program.get(position).get(GuideListView.ITEM_3);
                    if (description != null) {
                        tx_program_description.setText(description.toString());
                    } else {
                        tx_program_description.setText(mResources.getString(R.string.no_information));
                    }
                }
                break;
        }
    }

    private void showGuideToast(String status) {
        if (guide_toast == null) {
            guide_toast = Toast.makeText(this, status, Toast.LENGTH_SHORT);
        } else {
            guide_toast.setText(status);
        }
        guide_toast.show();
    }

    private void sendSwitchChannelBroadcast(int position) {
        int channelIndex = (int)list_channels.get(position).get(GuideListView.ITEM_2);
        boolean isRadio = (boolean)list_channels.get(position).get(GuideListView.ITEM_3);
        ChannelInfo currentChannel = channelInfoList.get(currentChannelIndex);
        String inputId = currentChannel.getInputId();
        int deviceId = getDeviceId(inputId);
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_SWITCH_CHANNEL);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_NUMBER, channelIndex);
        intent.putExtra(DroidLogicTvUtils.EXTRA_IS_RADIO_CHANNEL, isRadio);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_DEVICE_ID,deviceId);
        sendBroadcast(intent);
    }

    private int getDeviceId(String inputId) {
        String[] temp = inputId.split("/");
        if (temp.length == 3) {
            /*  ignore for HDMI CEC device */
            if (temp[2].contains("HDMI"))
                return -1;
            return Integer.parseInt(temp[2].substring(2));
        } else {
            return -1;
        }
    }

    private String getdate(long dateTime) {
        SimpleDateFormat sDateFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        sDateFormat.setTimeZone(TimeZone.getDefault());
        return sDateFormat.format(new Date(dateTime + 0));
    }

    public String[] getDateAndTime(long dateTime) {
        SimpleDateFormat sDateFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        sDateFormat.setTimeZone(TimeZone.getDefault());
        String[] dateAndTime = sDateFormat.format(new Date(dateTime + 0)).split("\\/| |:");

        return dateAndTime;
    }

    private String setAppointedProgramAlarm(Program currentProgram) {
        AlarmManager alarm = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
        String cancelProgram = "";

        List<Program> programList = mTvDataBaseManager.getAppointedPrograms();
        for (Program program : programList) {
            if (Math.abs(currentProgram.getStartTimeUtcMillis() - program.getStartTimeUtcMillis()) <= PROGRAM_INTERVAL) {
                if (cancelProgram.length() == 0) {
                    cancelProgram = mResources.getString(R.string.cancel) + " " + program.getTitle();
                } else {
                    cancelProgram += " " +  program.getTitle();
                }
                cancelAppointedProgramAlarm(program);
                program.setIsAppointed(false);
                mTvDataBaseManager.updateProgram(program);
            }
        }

        long pendingTime = currentProgram.getStartTimeUtcMillis() - mTvTime.getTime();
        if (pendingTime > 0) {
            Log.d(TAG, "" + pendingTime / 60000 + " min later show program prompt");
            alarm.setExact(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + pendingTime, buildPendingIntent(currentProgram));
        }

        if (cancelProgram.length() == 0) {
            return cancelProgram;
        } else {
            return "," + cancelProgram;
        }
    }

    private void cancelAppointedProgramAlarm (Program program) {
        AlarmManager alarm = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
        alarm.cancel(buildPendingIntent(program));
    }

    private PendingIntent buildPendingIntent (Program program) {
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_PROGRAM_APPOINTED);
        intent.putExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, program.getId());
        //sendBroadcast(intent);
        return PendingIntent.getBroadcast(this, (int)program.getId(), intent, 0);
    }

    private final class ProgramObserver extends ContentObserver {
        public ProgramObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (currentChannelIndex != -1) {
                ChannelInfo currentChannel = channelInfoList.get(currentChannelIndex);
                Program program = mTvDataBaseManager.getProgram(uri);
                if (program != null &&
                        program.getChannelId() == currentChannel.getId()) {
                    Log.d(TAG, "current channel update");
                    handler.removeMessages(MSG_LOAD_DATE);
                    handler.removeMessages(MSG_LOAD_PROGRAM);
                    Message msg = handler.obtainMessage(MSG_LOAD_DATE, -1, 0);
                    handler.sendMessageDelayed(msg, 500);
                }
            }
        }

        /*
        @Override
        public IContentObserver releaseContentObserver() {
            // TODO Auto-generated method stub
            return super.releaseContentObserver();
        }*/
    }

    private final class ChannelObserver extends ContentObserver {
        public ChannelObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (DroidLogicTvUtils.matchsWhich(uri) == DroidLogicTvUtils.MATCH_CHANNEL) {
                Log.d(TAG, "channel changed =" + uri);
                handler.removeMessages(MSG_LOAD_CHANNELS);
                handler.removeMessages(MSG_LOAD_DATE);
                handler.removeMessages(MSG_LOAD_PROGRAM);
                handler.sendEmptyMessageDelayed(MSG_LOAD_CHANNELS, 500);
            }
        }

        /*
        @Override
        public IContentObserver releaseContentObserver() {
            // TODO Auto-generated method stub
            return super.releaseContentObserver();
        }*/
    }

}
