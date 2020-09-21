/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ShortCutActivity
 */

package com.droidlogic.droidlivetv.shortcut;

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
import com.droidlogic.droidlivetv.shortcut.GuideListView.ListItemSelectedListener;
import com.droidlogic.droidlivetv.ui.MarqueeTextView;

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
import android.os.HandlerThread;
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
import android.widget.Button;
import android.widget.Toast;
import android.media.tv.TvContentRating;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.WindowManager;
import android.util.TypedValue;

import com.droidlogic.droidlivetv.R;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Arrays;
import java.util.TimeZone;
import java.text.SimpleDateFormat;
import java.util.Iterator;
import java.util.Comparator;
import java.util.Collections;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

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
    private static final String KEY_MENU_TIME = DroidLogicTvUtils.KEY_MENU_TIME;
    private static final int DEFUALT_MENU_TIME = DroidLogicTvUtils.DEFUALT_MENU_TIME;

    private static final long DAY_TO_MS = 86400000;
    private static final long PROGRAM_INTERVAL = 60000;

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
    private TextView tx_program_event_name;
    private TextView tx_program_genre;
    private TextView tx_program_rating;
    private TextView tx_program_extend;
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
    private final int INTERVAL = 20;//100MS

    private int mDeviceId = -1;
    private long mCurrentChannelId = -1;
    private String mCurrentInputId = null;
    private int mCurrentKeyCode = -1;
    private boolean mHandleUpdate = true;

    private final BroadcastReceiver mDroidLiveTvReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "intent = " + intent);
            if (intent != null) {
                if (DroidLogicTvUtils.ACTION_STOP_EPG_ACTIVITY.equals(intent.getAction())) {
                    finish();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        Intent intent = getIntent();
        Bundle bundle= intent.getExtras();
        registerMainReceiver();
        if (bundle != null) {
            mCurrentKeyCode = bundle.getInt("eventkey");
            mDeviceId = bundle.getInt("deviceid");
            mCurrentChannelId = bundle.getLong("channelid");
            mCurrentInputId = bundle.getString("inputid");
            Log.d(TAG, "onCreate keyvalue: " + mCurrentKeyCode + ", deviceid: " + mDeviceId + ", channelid: " + mCurrentChannelId + ", input: " + mCurrentInputId);
            mTvDataBaseManager = new TvDataBaseManager(this);
            mResources = getResources();
            setShortcutMode(mCurrentKeyCode);
            startShowActivityTimer();
        } else {
            finish();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "------onStart");
    }

    @Override
    protected void onResume() {
        super.onPause();
        Log.d(TAG, "------onResume");
        registerCommandReceiver();
    };

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "------onPause");
        unregisterCommandReceiver();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "------onStop");
        if (mProgramObserver != null) {
            getContentResolver().unregisterContentObserver(mProgramObserver);
            mProgramObserver = null;
        }
        if (mChannelObserver != null) {
            getContentResolver().unregisterContentObserver(mChannelObserver);
            mChannelObserver = null;
        }
        handler.removeCallbacksAndMessages(null);
        mThreadHandler.removeCallbacksAndMessages(null);
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "------onDestroy");
        unregisterMainReceiver();
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyUp(" + keyCode + ", " + event + ")");
        switch (keyCode) {
            case KeyEvent.KEYCODE_GUIDE:
                if (mTvTime != null) {
                    handler.sendEmptyMessageDelayed(MSG_FINISH, 0);
                }
                return true;
            case KeyEvent.KEYCODE_BACK:
                if (mTvTime != null) {
                    handler.sendEmptyMessageDelayed(MSG_FINISH, 0);
                }
                return true;
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_DPAD_CENTER:
                startShowActivityTimer();
                break;
            default:
                // pass through
        }
        return super.onKeyUp(keyCode, event);
    }

    private void setShortcutMode (int mode) {
        switch (mode) {
            case KeyEvent.KEYCODE_GUIDE:
                setContentView(R.layout.layout_shortcut_guide);
                setGuideView();
                break;
            default:
                break;
        }
    }

    private void startShowActivityTimer () {
        handler.removeMessages(MSG_FINISH);
        int seconds = Settings.System.getInt(getContentResolver(), KEY_MENU_TIME, DEFUALT_MENU_TIME);
        if (seconds == 0) {
            seconds = 15;
        } else if (seconds == 1) {
            seconds = 30;
        } else if (seconds == 2) {
            seconds = 60;
        } else if (seconds == 3) {
            seconds = 120;
        } else if (seconds == 4) {
            seconds = 240;
        } else {
            seconds = 0;
        }
        Log.d(TAG, "[startShowActivityTimer] seconds = " + seconds);
        if (seconds > 0) {
            handler.sendEmptyMessageDelayed(MSG_FINISH, seconds * 1000);
        } else {
            handler.removeMessages(MSG_FINISH);
        }
    }

    private void registerMainReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_TIME_TICK);
        filter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        filter.addAction(Intent.ACTION_TIME_CHANGED);
        filter.addAction(DroidLogicTvUtils.ACTION_DROID_PROGRAM_RECORD_STATUS);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        registerReceiver(mReceiver, filter);
    }

    private void unregisterMainReceiver() {
        unregisterReceiver(mReceiver);
    }

    private void registerCommandReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(DroidLogicTvUtils.ACTION_STOP_EPG_ACTIVITY);
        registerReceiver(mDroidLiveTvReceiver, intentFilter);
    }

    private void unregisterCommandReceiver() {
        unregisterReceiver(mDroidLiveTvReceiver);
    }

    Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FINISH:
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
                default:
                    break;
            }
        }
    };

    private HandlerThread mHandlerThread;
    private Handler  mThreadHandler;

    private void initHandlerThread() {
        mHandlerThread = new HandlerThread("check-message-coming");
        mHandlerThread.start();
        mThreadHandler = new Handler(mHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_LOAD_CHANNELS:
                        try {
                            loadChannelList();
                        } catch (RuntimeException e) {
                            e.printStackTrace();
                        }
                        break;
                    case MSG_LOAD_DATE:
                        if (msg.arg1 != -1) {
                            currentDateIndex = -1;
                            currentProgramIndex = -1;
                        }
                        Log.d(TAG, "current Date:" + currentDateIndex);
                        try {
                            loadDateList();
                        } catch (RuntimeException e) {
                            e.printStackTrace();
                        }
                        break;
                    case MSG_LOAD_PROGRAM:
                        Log.d(TAG, "current Program:" + currentProgramIndex);
                        try {
                            loadProgramList();
                        } catch (RuntimeException e) {
                            e.printStackTrace();
                        }
                        break;
                    default:
                        break;
                }
            }
        };
    }

    BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_TIME_TICK.equals(action)) {
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
            } else if (DroidLogicTvUtils.ACTION_DROID_PROGRAM_RECORD_STATUS.equals(action)) {
                Log.d(TAG, "REQUEST_RECORD_STATUS " + intent.toString());
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                Log.d(TAG, "Receive ACTION_SCREEN_OFF");
                finish();//stop epg when before suspend
            }
        }
    };

    public static class CompareDisplayNumber implements Comparator<ChannelInfo> {

        @Override
        public int compare(ChannelInfo o1, ChannelInfo o2) {
            int result = compareString(o1.getDisplayNumber(), o2.getDisplayNumber());
            return result;
        }

        private int compareString(String a, String b) {
            if (a == null) {
                return b == null ? 0 : -1;
            }
            if (b == null) {
                return 1;
            }

            int[] disnumbera = getMajorAndMinor(a);
            int[] disnumberb = getMajorAndMinor(b);
            if (disnumbera[0] != disnumberb[0]) {
                return (disnumbera[0] - disnumberb[0]) > 0 ? 1 : -1;
            } else if (disnumbera[1] != disnumberb[1]) {
                return (disnumbera[1] - disnumberb[1]) > 0 ? 1 : -1;
            }
            return 0;
        }

        private int[] getMajorAndMinor(String disnumber) {
            int[] result = {-1, -1};//major, minor
            String[] splitone = (disnumber != null ? disnumber.split("-") : null);
            if (splitone != null && splitone.length > 0) {
                int length = 2;
                if (splitone.length <= 2) {
                    length = splitone.length;
                } else {
                    Log.d(TAG, "informal disnumber");
                    return result;
                }
                for (int i = 0; i < length; i++) {
                    try {
                       result[i] = Integer.valueOf(splitone[i]);
                    } catch (NumberFormatException e) {
                        Log.d(TAG, splitone[i] + " not integer:" + e.getMessage());
                    }
                }
            }
            return result;
        }
    }

    private void setGuideView() {
        mTvTime = new TvTime(this);

        loadDateTime();

        tx_program_description = (TextView)findViewById(R.id.program_short/*guide_details_content*/);
        tx_program_event_name = (TextView)findViewById(R.id.program_event);
        tx_program_genre = (TextView)findViewById(R.id.program_genre);
        tx_program_rating = (TextView)findViewById(R.id.program_rating);
        tx_program_extend = (TextView)findViewById(R.id.program_extend);
        //tx_program_event_name.setVisibility(View.GONE);//add if needed

        lv_channel = (GuideListView)findViewById(R.id.list_guide_channel);
        lv_date = (GuideListView)findViewById(R.id.list_guide_week);
        lv_program = (GuideListView)findViewById(R.id.list_guide_programs);

        lv_channel.setListItemSelectedListener(this);
        lv_channel.setOnItemClickListener(this);

        lv_date.setListItemSelectedListener(this);

        lv_program.setListItemSelectedListener(this);
        lv_program.setOnItemClickListener(this);
        initHandlerThread();

        if (mProgramObserver == null)
            mProgramObserver = new ProgramObserver();
        if (mChannelObserver == null)
            mChannelObserver = new ChannelObserver();

        getContentResolver().registerContentObserver(TvContract.Programs.CONTENT_URI, true, mProgramObserver);
        getContentResolver().registerContentObserver(TvContract.Channels.CONTENT_URI, true, mChannelObserver);

        mThreadHandler.sendEmptyMessage(MSG_LOAD_CHANNELS);
    }

    private void loadDateTime() {
        tx_date = (TextView)findViewById(R.id.guide_date);
        String[] dateAndTime = getDateAndTime(mTvTime.getTime());
        tx_date.setText(dateAndTime[0] + "." + dateAndTime[1] + "." + dateAndTime[2] + "   " + dateAndTime[3] + ":" + dateAndTime[4]);
    }

    public ArrayList<ArrayMap<String, Object>> getDTVChannelList (ArrayList<ChannelInfo> channelInfoList) {
        ArrayList<ArrayMap<String, Object>> list =  new ArrayList<ArrayMap<String, Object>>();
        int dtvchannelindex = 0;
        if (channelInfoList.size() > 0) {
            for (int i = 0 ; i < channelInfoList.size(); i++) {
                ChannelInfo info = channelInfoList.get(i);
                if (info != null && !info.isAnalogChannel()) {
                    ArrayMap<String, Object> item = new ArrayMap<String, Object>();
                    String localName = info.getDisplayNameLocal();
                    String displayName = info.getDisplayName();
                    item.put(GuideListView.ITEM_1, info.getDisplayNumber() + "." + (!TextUtils.isEmpty(localName) ? localName : displayName));
                    item.put(GuideListView.ITEM_2, info.getDisplayNumber());
                    if (ChannelInfo.isRadioChannel(info)) {
                        item.put(GuideListView.ITEM_3, true);
                    } else {
                        item.put(GuideListView.ITEM_3, false);
                    }
                    list.add(item);
                    if (mCurrentChannelId == info.getId())
                        currentChannelIndex = dtvchannelindex;
                    dtvchannelindex++;
                }
            }
        }
        return list;
    }

    private void loadChannelList() {
        Log.d(TAG, "load Channels");

        if (mCurrentInputId == null) {
            return;
        }

        list_channels.clear();
        String searchedId = DroidLogicTvUtils.getSearchInputId(this);
        if (searchedId != null && searchedId.startsWith("com.droidlogic.tvinput")) {
            channelInfoList = mTvDataBaseManager.getChannelList(mCurrentInputId, Channels.SERVICE_TYPE_AUDIO_VIDEO, true, false);//hide channel is not needed
            channelInfoList.addAll(mTvDataBaseManager.getChannelList(mCurrentInputId, Channels.SERVICE_TYPE_AUDIO, true, false));
        } else {
            channelInfoList = mTvDataBaseManager.getChannelList(mCurrentInputId, Channels.SERVICE_TYPE_AUDIO_VIDEO, false, false);
            channelInfoList.addAll(mTvDataBaseManager.getChannelList(mCurrentInputId, Channels.SERVICE_TYPE_AUDIO, false, false));
        }

        if (channelInfoList != null && channelInfoList.size() > 0) {
            Collections.sort(channelInfoList, new CompareDisplayNumber());
        }
        Iterator it = channelInfoList.iterator();
        ChannelInfo channel = null;
        while (it.hasNext()) {
            channel = (ChannelInfo)it.next();
            if (channel != null && channel.isAnalogChannel()) {
                it.remove();
            }
        }

        list_channels = getDTVChannelList(channelInfoList);
        Log.d(TAG, "loadChannelList list_channels.size() = " + list_channels.size());

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
            long currentStreamTime = mTvTime.getTime();
            int time_offset = TimeZone.getDefault().getOffset(currentStreamTime);
            long tmp_time = (currentStreamTime) - ((currentStreamTime + time_offset) % DAY_TO_MS);
            int count = 0;
            Log.d(TAG, "currentStreamTime = " + Arrays.toString(getDateAndTime(tmp_time)) +
                    ", firstProgramTime = " + Arrays.toString(getDateAndTime(firstProgramTime)) +
                    ", lastProgramTime = " + Arrays.toString(getDateAndTime(lastProgramTime)));
            while ((tmp_time <= lastProgramTime) && count < 10) {//show 1 + 8 days
                if (currentDateIndex == -1) {
                    if (currentStreamTime/*mTvTime.getTime()*/ >= tmp_time && currentStreamTime/*mTvTime.getTime()*/ < tmp_time + DAY_TO_MS)
                        currentDateIndex = count;
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
                if (tmp_time <= currentStreamTime/*mTvTime.getTime()*/) {
                    continue;
                }
                count++;
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
                String watchStatus = "";
                String pvrStatus = "";

                ArrayMap<String, Object> item_program = new ArrayMap<String, Object>();

                item_program.put(GuideListView.ITEM_1, dateAndTime[3] + ":" + dateAndTime[4]
                                 + "~" + endTime[3] + ":" + endTime[4]);
                item_program.put(GuideListView.ITEM_2, program.getTitle());
                String descrip = program.getDescription();
                String longDescrip = program.getLongDescription();
                String genresStr = null;
                String ratingsStr = null;
                String[] genres = program.getCanonicalGenres();
                TvContentRating[] ratings = program.getContentRatings();
                if (genres != null && genres.length > 0) {
                    for (String sub : genres) {
                        if (genresStr == null) {
                            genresStr = sub;
                        } else {
                            genresStr = genresStr + " " + sub;
                        }
                    }
                }
                if (ratings != null && ratings.length > 0) {
                    for (TvContentRating sub : ratings) {
                        if (ratingsStr == null) {
                            ratingsStr = sub.getMainRating();
                        } else {
                            ratingsStr = ratingsStr + " " + sub.getMainRating();
                        }
                    }
                }
                JSONObject jsonObj = new JSONObject();
                try {
                    jsonObj.put("program_event", program.getTitle());
                    jsonObj.put("program_genre", genresStr != null ? genresStr : "");
                    jsonObj.put("program_rating", ratingsStr != null ? ratingsStr : "");
                    jsonObj.put("program_short", program.getDescription());
                    jsonObj.put("program_extend", program.getLongDescription());
                } catch (JSONException e) {
                    Log.e(TAG, "load Program JSONException = " + e.getMessage());
                    e.printStackTrace();
                }
                String des = jsonObj.toString();
                /*if (!TextUtils.isEmpty(descrip)) {
                    if (!TextUtils.isEmpty(longDescrip)) {
                        des = descrip + "\n" + longDescrip;
                    } else {
                        des = descrip;
                    }
                } else if (TextUtils.isEmpty(descrip)) {
                    des = longDescrip;
                }*/
                //Log.d(TAG, "loadProgramList descrip = " + descrip + "\n" + ", longDescrip = " + longDescrip);
                item_program.put(GuideListView.ITEM_3, des);
                item_program.put(GuideListView.ITEM_4, Long.toString(program.getId()));

                if (mTvTime.getTime() >= program.getStartTimeUtcMillis() && mTvTime.getTime() <= program.getEndTimeUtcMillis()) {
                    if (currentProgramIndex == -1)
                        currentProgramIndex = i;
                    watchStatus = GuideListView.STATUS_PLAYING;
                    if (program.getScheduledRecordStatus() == Program.RECORD_STATUS_IN_PROGRESS) {
                        pvrStatus = GuideListView.STATUS_RECORDING;
                    }
                } else {
                    if (program.getScheduledRecordStatus() == Program.RECORD_STATUS_APPOINTED) {
                        pvrStatus = GuideListView.STATUS_APPOINTED_RECORD;
                    } else {
                        pvrStatus = "";
                    }
                    if (program.isAppointed()) {
                        watchStatus = GuideListView.STATUS_APPOINTED_WATCH;
                    } else {
                        watchStatus = "";
                    }
                }
                item_program.put(GuideListView.ITEM_5, watchStatus);
                item_program.put(GuideListView.ITEM_6, pvrStatus);

                if (saveChannelIndex != currentChannelIndex) {
                    return;
                }
                list_program.add(item_program);
            }
        }
        if (list_program.size() == 0) {
            ArrayMap<String, Object> item = new ArrayMap<String, Object>();
            item.put(GuideListView.ITEM_1, mResources.getString(R.string.no_program));
            item.put(GuideListView.ITEM_2, "");
            item.put(GuideListView.ITEM_3, "");
            item.put(GuideListView.ITEM_4, "");
            item.put(GuideListView.ITEM_5, "");
            item.put(GuideListView.ITEM_6, "");

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
        mHandleUpdate = false;
    }


    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        //Log.d(TAG, "onItemClick parent = " + parent + ", view = " + view + ",position = " + position);
        switch (parent.getId()) {
            case R.id.list_guide_channel:
                sendSwitchChannelBroadcast(position);
                break;
            case R.id.list_guide_programs:
                if (list_program.size() > position) {
                    Object programId_object = list_program.get(position).get(GuideListView.ITEM_4);
                    if (programId_object != null) {
                        String idString = list_program.get(position).get(GuideListView.ITEM_4).toString();
                        if (!TextUtils.isEmpty(idString)) {
                            long programId = Long.valueOf(idString);
                            Program program = mTvDataBaseManager.getProgram(programId);
                            if (program != null) {
                                long streamTime = mTvTime.getTime();
                                if (streamTime >= program.getStartTimeUtcMillis() && streamTime <= program.getEndTimeUtcMillis()) {
                                    Log.d(TAG, "onItemClick current playing program can't be appointed");
                                    String canot_appointed_status = mResources.getString(R.string.appointed_donot_appoint_current);
                                    showGuideToast(canot_appointed_status);
                                } else {
                                    showDialogToAppoint(ShortCutActivity.this, view, program);
                                }
                            } else {
                                Log.d(TAG, "onItemClick current appointed program can't be found");
                            }
                        }
                    }
                }
        }
    }

    @Override
    public void onListItemSelected(View parent, int position) {
        switch (parent.getId()) {
            case R.id.list_guide_channel:
                if (parent != null && parent instanceof GuideListView) {
                    GuideListView listView = (GuideListView)parent;
                    int count = listView.getChildCount();
                    View last = null;
                    View next = null;
                    View current = (View)listView.getSelectedView();
                    if (position > 0) {
                        last = listView.getChildAt(position - 1);
                    }
                    if (position < count - 1) {
                        next = listView.getChildAt(position + 1);
                    }
                    if (last != null) {
                        MarqueeTextView marqueeText = last.findViewById(R.id.text_name);
                        if (marqueeText != null) {
                            marqueeText.setFocused(false);
                        }
                    }
                    if (next != null) {
                        MarqueeTextView marqueeText = next.findViewById(R.id.text_name);
                        if (marqueeText != null) {
                            marqueeText.setFocused(false);
                        }
                    }
                    if (current != null) {
                        MarqueeTextView marqueeText = current.findViewById(R.id.text_name);
                        if (marqueeText != null) {
                            marqueeText.setFocused(true);
                        }
                    }
                }
                lv_date.setAdapter(null);
                lv_program.setAdapter(null);
                currentChannelIndex = position;
                mThreadHandler.removeMessages(MSG_LOAD_DATE);
                mThreadHandler.removeMessages(MSG_LOAD_PROGRAM);
                mThreadHandler.sendEmptyMessageDelayed(MSG_LOAD_DATE, INTERVAL);
                mHandleUpdate = true;
                break;
            case R.id.list_guide_week:
                currentDateIndex = position;
                mThreadHandler.removeMessages(MSG_LOAD_PROGRAM);
                mThreadHandler.sendEmptyMessageDelayed(MSG_LOAD_PROGRAM, INTERVAL);
                mHandleUpdate = true;
                break;
            case R.id.list_guide_programs:
                if (position < list_program.size()) {
                    currentProgramIndex = position;
                    String jsonObjStr = (String)list_program.get(position).get(GuideListView.ITEM_3);
                    /*if (description != null) {
                        tx_program_description.setText(description.toString());
                    } else {
                        tx_program_description.setText(mResources.getString(R.string.no_information));
                    }*/
                    String program_event = null;
                    String program_genre = null;
                    String program_rating = null;
                    String program_short = null;
                    String program_extend = null;
                    if (TextUtils.isEmpty(jsonObjStr)) {
                        tx_program_event_name.setText(getString(R.string.program_event_name) + "\n" + getString(R.string.no_information));
                        tx_program_genre.setText(getString(R.string.program_genre) + "\n" + getString(R.string.no_information));
                        tx_program_rating.setText(getString(R.string.program_rating) + "\n" + getString(R.string.no_information));
                        tx_program_description.setText(getString(R.string.program_short) + "\n" + getString(R.string.no_information));
                        tx_program_extend.setText(getString(R.string.program_extend) + "\n" + getString(R.string.no_information));
                    } else {
                        try {
                            JSONObject jsonObj = new JSONObject(jsonObjStr.toString());
                            program_event = jsonObj.getString("program_event");
                            program_genre = jsonObj.getString("program_genre");
                            program_rating = jsonObj.getString("program_rating");
                            program_short = jsonObj.getString("program_short");
                            program_extend = jsonObj.getString("program_extend");
                        } catch (JSONException e) {
                            Log.e(TAG, "onListItemSelected list_guide_programs JSONException = " + e.getMessage());
                            e.printStackTrace();
                        } finally {
                            if (TextUtils.isEmpty(program_event)) {
                                tx_program_event_name.setText(getString(R.string.program_event_name) + "\n" + getString(R.string.no_information));
                            } else {
                                tx_program_event_name.setText(getString(R.string.program_event_name) + "\n" + program_event);
                            }
                            if (TextUtils.isEmpty(program_genre)) {
                                tx_program_genre.setText(getString(R.string.program_genre) + "\n" + getString(R.string.no_information));
                            } else {
                                tx_program_genre.setText(getString(R.string.program_genre) + "\n" + program_genre);
                            }
                            if (TextUtils.isEmpty(program_rating)) {
                                tx_program_rating.setText(getString(R.string.program_rating) + "\n" + getString(R.string.no_information));
                            } else {
                                tx_program_rating.setText(getString(R.string.program_rating) + "\n" + program_rating);
                            }
                            if (TextUtils.isEmpty(program_short)) {
                                tx_program_description.setText(getString(R.string.program_short) + "\n" + getString(R.string.no_information));
                            } else {
                                tx_program_description.setText(getString(R.string.program_short) + "\n" + program_short);
                            }
                            if (TextUtils.isEmpty(program_extend)) {
                                tx_program_extend.setText(getString(R.string.program_extend) + "\n" + getString(R.string.no_information));
                            } else {
                                tx_program_extend.setText(getString(R.string.program_extend) + "\n" + program_extend);
                            }
                        }
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

    private void showDialogToAppoint(final Context context, final View view, final Program program) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog alert = builder.create();
        final View dialogView = View.inflate(context, R.layout.watch_record_confirm, null);
        final Button watch = (Button) dialogView.findViewById(R.id.dialog_watch);
        final Button record = (Button) dialogView.findViewById(R.id.dialog_record);
        watch.requestFocus();
        handler.removeMessages(MSG_FINISH);

        watch.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                //appoint to watch
                dealAppoint(context, program, view, true);
                alert.dismiss();
            }
        });
        record.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                //appoint to record
                dealAppoint(context, program, view, false);
                alert.dismiss();
            }
        });
        alert.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "showDialogToAppoint onDismiss");
                startShowActivityTimer();
            }
        });
        alert.setView(dialogView);
        alert.show();
        //set size and background
        WindowManager.LayoutParams params = alert.getWindow().getAttributes();
        params.width = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 400, getResources().getDisplayMetrics());
        params.height = WindowManager.LayoutParams.WRAP_CONTENT ;
        alert.getWindow().setAttributes(params);
        alert.getWindow().setBackgroundDrawableResource(R.drawable.dialog_background);
    }

    private void showOverlapConfirmDialog(final Context context, final View view, final Program overlapProgram, final Program newProgram) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog alert = builder.create();
        final View dialogView = View.inflate(context, R.layout.overlap_confirm, null);
        final TextView title = (TextView) dialogView.findViewById(R.id.dialog_title);
        final Button cancel = (Button) dialogView.findViewById(R.id.dialog_cancel);
        final Button add = (Button) dialogView.findViewById(R.id.dialog_add);
        final String[] startTime = getDateAndTime(overlapProgram.getStartTimeUtcMillis());
        final String[] endTime = getDateAndTime(overlapProgram.getEndTimeUtcMillis());
        final String timePeriod = startTime[3] + ":" + startTime[4] + "~" +
                endTime[3] + ":" + endTime[4];
        title.setText(mResources.getString(R.string.appointed_overlap_title, overlapProgram.getTitle(), timePeriod));
        cancel.requestFocus();
        handler.removeMessages(MSG_FINISH);

        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                alert.dismiss();
            }
        });
        add.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                newProgram.setScheduledRecordStatus(Program.RECORD_STATUS_APPOINTED);
                ((ImageView)view.findViewById(R.id.img_appointed)).setImageResource(R.drawable.scheduled_recording);
                String appointed_status = mResources.getString(R.string.appointed_success) + setAppointedRecordProgram(newProgram);
                mTvDataBaseManager.updateProgram(newProgram);
                alert.dismiss();
            }
        });
        alert.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "showOverlapConfirmDialog onDismiss");
                startShowActivityTimer();
            }
        });
        alert.setView(dialogView);
        alert.show();
        //set size and background
        WindowManager.LayoutParams params = alert.getWindow().getAttributes();
        params.width = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 600, getResources().getDisplayMetrics());
        params.height = WindowManager.LayoutParams.WRAP_CONTENT ;
        alert.getWindow().setAttributes(params);
        alert.getWindow().setBackgroundDrawableResource(R.drawable.dialog_background);
    }

    private void showAppointWatchConflictDialog(final Context context, final View view, final Program overlapProgram, final Program newProgram) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog alert = builder.create();
        final View dialogView = View.inflate(context, R.layout.overlap_confirm, null);
        final TextView title = (TextView) dialogView.findViewById(R.id.dialog_title);
        final Button cancel = (Button) dialogView.findViewById(R.id.dialog_cancel);
        final Button add = (Button) dialogView.findViewById(R.id.dialog_add);
        final String[] startTime = getDateAndTime(overlapProgram.getStartTimeUtcMillis());
        final String[] endTime = getDateAndTime(overlapProgram.getEndTimeUtcMillis());
        final String timePeriod = startTime[3] + ":" + startTime[4] + "~" +
                endTime[3] + ":" + endTime[4];
        cancel.setText(R.string.appointed_watch_conflict_cancel);
        add.setText(R.string.appointed_watch_conflict_replace);
        title.setText(mResources.getString(R.string.appointed_watch_conflict, overlapProgram.getTitle(), timePeriod));
        add.requestFocus();
        handler.removeMessages(MSG_FINISH);

        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                alert.dismiss();
            }
        });
        add.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                newProgram.setIsAppointed(true);
                ((ImageView)view.findViewById(R.id.img_playing)).setImageResource(R.drawable.appointed);
                String appointed_status = mResources.getString(R.string.appointed_success) + setAppointedProgramAlarm(newProgram);
                mTvDataBaseManager.updateProgram(newProgram);
                showGuideToast(appointed_status);
                alert.dismiss();
            }
        });
        alert.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "showAppointWatchConflictDialog onDismiss");
                startShowActivityTimer();
            }
        });
        alert.setView(dialogView);
        alert.show();
        //set size and background
        WindowManager.LayoutParams params = alert.getWindow().getAttributes();
        params.width = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 600, getResources().getDisplayMetrics());
        params.height = WindowManager.LayoutParams.WRAP_CONTENT ;
        alert.getWindow().setAttributes(params);
        alert.getWindow().setBackgroundDrawableResource(R.drawable.dialog_background);
    }

    private void dealAppoint(Context context, Program program, View view, boolean isWatch) {
        String appointed_status;
        if (program != null && view != null) {
            if (isWatch) {
                if (mTvTime.getTime() < program.getStartTimeUtcMillis()) {
                    if (program.isAppointed()) {
                        program.setIsAppointed(false);
                        ((ImageView)view.findViewById(R.id.img_playing)).setImageResource(0);
                        appointed_status = mResources.getString(R.string.appointed_cancel);
                        mTvDataBaseManager.updateProgram(program);
                        cancelAppointedProgramAlarm(program);
                        showGuideToast(appointed_status);
                    } else {
                        Program conflictProgram = getAppointedTimeConflictedProgram(program.getStartTimeUtcMillis());
                        if (conflictProgram == null) {
                            program.setIsAppointed(true);
                            ((ImageView)view.findViewById(R.id.img_playing)).setImageResource(R.drawable.appointed);
                            appointed_status = mResources.getString(R.string.appointed_success) + setAppointedProgramAlarm(program);
                            mTvDataBaseManager.updateProgram(program);
                            showGuideToast(appointed_status);
                        } else {
                            showAppointWatchConflictDialog(context, view, conflictProgram, program);
                        }
                    }
                } else {
                    appointed_status = mResources.getString(R.string.appointed_expired);
                    showGuideToast(appointed_status);
                }
            } else {
                if (mTvTime.getTime() < program.getStartTimeUtcMillis()) {
                    if (program.getScheduledRecordStatus() == Program.RECORD_STATUS_APPOINTED) {
                        program.setScheduledRecordStatus(Program.RECORD_STATUS_NOT_STARTED);
                        ((ImageView)view.findViewById(R.id.img_appointed)).setImageResource(0);
                        appointed_status = mResources.getString(R.string.appointed_cancel);
                        mTvDataBaseManager.updateProgram(program);
                        cancelAppointedRecordProgram(program);
                    } else if (program.getScheduledRecordStatus() == Program.RECORD_STATUS_NOT_STARTED) {
                        List<Program> overlapPrograms = mTvDataBaseManager.getOverlapProgramsInTimePeriod(program);
                        if (overlapPrograms != null && overlapPrograms.size() > 0) {
                            Log.d(TAG, "dealAppoint record find overlap schedule");
                            showOverlapConfirmDialog(context, view, overlapPrograms.get(0), program);
                        } else {
                            program.setScheduledRecordStatus(Program.RECORD_STATUS_APPOINTED);
                            ((ImageView)view.findViewById(R.id.img_appointed)).setImageResource(R.drawable.scheduled_recording);
                            appointed_status = mResources.getString(R.string.appointed_success) + setAppointedRecordProgram(program);
                            mTvDataBaseManager.updateProgram(program);
                        }
                    }
                } else {
                    appointed_status = mResources.getString(R.string.appointed_expired);
                }
                //showGuideToast(appointed_status);
            }
        }
    }

    private void sendSwitchChannelBroadcast(int position) {
        boolean isRadio = (boolean)list_channels.get(position).get(GuideListView.ITEM_3);
        ChannelInfo currentChannel = channelInfoList.get(currentChannelIndex);
        String inputId = currentChannel.getInputId();
        int deviceId = mDeviceId;
        long channelid = -1;
        if (currentChannel != null) {
            channelid = currentChannel.getId();
        }
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_SWITCH_CHANNEL);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, channelid);
        intent.putExtra(DroidLogicTvUtils.EXTRA_IS_RADIO_CHANNEL, isRadio);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_DEVICE_ID, deviceId);
        sendBroadcast(intent);
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
            Log.d(TAG, "" + pendingTime / 60000 + " min " + pendingTime % 60000 / 1000 + " sec later show program prompt");
            //check it one minute before appointed time is up
            if (pendingTime > 60000) {
                pendingTime = pendingTime - 60000;
            } else {
                pendingTime = 0;
            }
            //PendingIntent pendingIntent = buildPendingIntent(currentProgram, true);
            //alarm.setExact(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + pendingTime, pendingIntent);
            //excuted by broadcast
            sendAppointedWatchIntent(currentProgram, true, pendingTime);
        }

        if (cancelProgram.length() == 0) {
            return cancelProgram;
        } else {
            return "," + cancelProgram;
        }
    }

    private void cancelAppointedProgramAlarm(Program program) {
        sendAppointedWatchIntent(program, false, -1l);
    }

    private String setAppointedRecordProgram(Program currentProgram) {
        String schedulerProgram = "";
        if (currentProgram != null) {
            schedulerProgram = currentProgram.getTitle();
            long pendingTime = currentProgram.getStartTimeUtcMillis() - mTvTime.getTime();
            if (pendingTime > 0) {
                Log.d(TAG, "setAppointedWatchProgram send scheduler " + pendingTime / 60000 + " min " + pendingTime % 60000 / 1000 + " sec later starts record");
                sendSchedulerIntent(currentProgram, true);
            }
        }
        if (schedulerProgram.length() == 0) {
            return schedulerProgram;
        } else {
            return " " + schedulerProgram;
        }
    }

    private void cancelAppointedRecordProgram(Program program) {
        if (program != null) {
            sendSchedulerIntent(program, false);
        }
    }

    private void sendAppointedWatchIntent (Program program, boolean add, long delay) {
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_DROID_PROGRAM_WATCH_APPOINTED);
        intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
        intent.setData(TvContract.buildProgramUri(program.getId()));
        intent.setClassName("com.droidlogic.droidlivetv", "com.droidlogic.droidlivetv.shortcut.AppointedProgramReceiver");
        intent.putExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, program.getId());
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, program.getChannelId());
        intent.putExtra(DroidLogicTvUtils.EXTRA_APPOINTED_DELAY, delay);
        intent.putExtra(DroidLogicTvUtils.EXTRA_APPOINTED_SETTING, true);
        intent.putExtra(DroidLogicTvUtils.EXTRA_APPOINTED_ACTION, add);
        sendBroadcast(intent);
        Log.d(TAG, "sendSettingAppointedWatchIntent intent = " + intent);
    }

    private void sendSchedulerIntent (Program program, boolean add) {
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_DROID_PROGRAM_RECORD_APPOINTED);
        intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
        intent.putExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, program.getId());
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, program.getChannelId());
        intent.putExtra(DroidLogicTvUtils.ACTION_DROID_PROGRAM_RECORD_OPERATION, add);
        sendBroadcast(intent);
    }

    private PendingIntent getExistPendingIntent (Program program) {
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_DROID_PROGRAM_RECORD_APPOINTED);
        intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
        intent.putExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, program.getId());
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, program.getChannelId());
        //sendBroadcast(intent);
        return PendingIntent.getBroadcast(this, (int)program.getId(), intent, PendingIntent.FLAG_NO_CREATE);
    }

    private Program getAppointedTimeConflictedProgram(long startTime) {
        Program result = null;
        List<Program> appointedPrograms = mTvDataBaseManager.getAppointedPrograms();
        if (appointedPrograms != null && appointedPrograms.size() > 0) {
            for (Program single : appointedPrograms) {
                if (startTime == single.getStartTimeUtcMillis()) {
                    result = single;
                    break;
                }
            }
        }
        return result;
    }

    private final class ProgramObserver extends ContentObserver {
        public ProgramObserver() {
            super(mThreadHandler/*new Handler()*/);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (mHandleUpdate) {
                return;
            }
            if (currentChannelIndex != -1) {
                ChannelInfo currentChannel = channelInfoList.get(currentChannelIndex);
                Program program = mTvDataBaseManager.getProgram(uri);
                if (program != null &&
                        program.getChannelId() == currentChannel.getId()) {
                    Log.d(TAG, "current channel update");
                    mThreadHandler.removeMessages(MSG_LOAD_DATE);
                    mThreadHandler.removeMessages(MSG_LOAD_PROGRAM);
                    Message msg = mThreadHandler.obtainMessage(MSG_LOAD_DATE, -1, 0);
                    mThreadHandler.sendMessageDelayed(msg, 500);
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
            super(mThreadHandler/*new Handler()*/);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (mHandleUpdate) {
                return;
            }
            if (DroidLogicTvUtils.matchsWhich(uri) == DroidLogicTvUtils.MATCH_CHANNEL) {
                Log.d(TAG, "channel changed =" + uri);
                mThreadHandler.removeMessages(MSG_LOAD_CHANNELS);
                mThreadHandler.removeMessages(MSG_LOAD_DATE);
                mThreadHandler.removeMessages(MSG_LOAD_PROGRAM);
                mThreadHandler.sendEmptyMessageDelayed(MSG_LOAD_CHANNELS, 500);
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
