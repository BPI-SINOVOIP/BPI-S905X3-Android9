package com.droidlogic.droidlivetv.favlistsettings;

import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
//import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.widget.AdapterView;
import android.widget.LinearLayout;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.media.tv.TvInputInfo;

import java.util.LinkedList;

import com.droidlogic.droidlivetv.favlistsettings.IInterationCallback;
import com.droidlogic.droidlivetv.favlistsettings.IInterationService;

import com.droidlogic.app.tv.DroidLogicTvUtils;

import com.droidlogic.droidlivetv.R;

public class SortFavActivity extends Activity {

    private static final String TAG = SortFavActivity.class.getSimpleName();
    private static final boolean DEBUG = true;
    private static final String LOGD = "logd";
    private static final String LOGE = "loge";

    private ChannelListListView mAllListView;
    private ChannelListListView mSortListView;
    private ChannelListListView mContentListView;
    private FavListListView mFavListView;
    private LinearLayout mRightShowContainner;

    private TextView mLeftTitle;
    private TextView mRightTitle;
    private Button mAllButton;
    private Button mATwoZButton;
    private Button mTPButton;
    private Button mNetWorkIdButton;
    private Button mExtraSortButton;
    private TextView mSatelliteText;
    private TextView mSatellite;
    private TextView mTransponder;

    private Button mFindButton;
    private Button mAddToFavButton;
    private Button mSatelliteButton;
    private Button mFavListButton;

    private ChannelDataManager mChannelDataManager;
    private CustomedDialogView mCustomedDialogView;
    private String mInputId = null;

    private static final int CONNECT = 0;
    private static final int UPDATE_FAV_GROUP = 1;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case CONNECT:
                    bindInterationService();
                    break;
                case UPDATE_FAV_GROUP:
                    dealFavGroupChange((Bundle)msg.obj);
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //requestWindowFeature(Window.FEATURE_NO_TITLE);
        //setContentView(R.layout.activity_main);
        setContentView(R.layout.channel_sort_edit);
        mHandler.sendEmptyMessage(CONNECT);
        mChannelDataManager = new ChannelDataManager(this);
        Intent intent = getIntent();
        if (intent != null) {
            mInputId = intent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        }
        mChannelDataManager.setInputId(mInputId);
        init();
        LOG(LOGD, null, "onCreate mInputId = " + mInputId);
    }

    private int mKeyDownCount = 0;

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        LOG(LOGD, null, "onKeyDown " + event);
        mKeyDownCount = event.getRepeatCount();
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            /*if (mRightShowContainner.getVisibility() == View.VISIBLE) {
                mRightShowContainner.setVisibility(View.GONE);
                return true;
            }*/
        }
        if (mKeyDownCount == 1) {//think it is long pressed
            if (keyCode == KeyEvent.KEYCODE_F4) {
                dealLongPressAction(getKeyEventAction(keyCode, event));
            }

        }
        return super.onKeyDown(keyCode, event);
    }

    /*@Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        LOG(LOGD, null, "onKeyLongPress " + event);
        if (keyCode == KeyEvent.KEYCODE_F4) {
            dealLongPressAction(getKeyEventAction(keyCode, event));
            return true;
        }
        return false;
    }*/

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        LOG(LOGD, null, "onKeyUp " + event);
        if (mKeyDownCount > 0) {
            mKeyDownCount = 0;
            return true;
        }
        mKeyDownCount = 0;
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            if (mRightShowContainner.getVisibility() == View.VISIBLE) {
                mRightShowContainner.setVisibility(View.GONE);
                if (mAllListView.getVisibility() == View.VISIBLE) {
                    mAllListView.requestFocus();
                } else if (mContentListView.getVisibility() == View.VISIBLE) {
                    mContentListView.requestFocus();
                } else if (mSortListView.getVisibility() == View.VISIBLE) {
                    mSortListView.requestFocus();
                }
                return true;
            } else if (mAllListView.hasFocus() || mContentListView.hasFocus() || mSortListView.hasFocus()) {
                mAllButton.requestFocus();
                return true;
            }
        }
        dealAction(getKeyEventAction(keyCode, event));
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            if (mClient != null) {
                mClient.unRegisterInterationCallback(mIInterationCallback);
            }
        } catch (Exception e) {
            Log.d(TAG, "onDestroy unRegisterInterationCallback e = " + e.getMessage());
            e.printStackTrace();
        }
        unbindInterationService();
        LOG(LOGD, null, "onDestroy");
    }

    private static final int ACTION_CHANNEL_SORT_ALL = 0;
    private static final int ACTION_CHANNEL_SORT_AZ = 1;
    private static final int ACTION_CHANNEL_SORT_TP = 2;
    private static final int ACTION_CHANNEL_SORT_NETWORKID = 3;
    private static final int ACTION_CHANNEL_SORT_SORT = 4;
    private static final int ACTION_FUNVTION_FIND = 5;
    private static final int ACTION_FUNVTION_ADD_FAV = 6;
    private static final int ACTION_FUNVTION_SATELLITE = 7;
    private static final int ACTION_FUNVTION_FAVLIST = 8;

    private int getKeyEventAction(int keyCode, KeyEvent event) {
        int resultAction = -1;
        switch (keyCode) {
            case KeyEvent.KEYCODE_PROG_RED:
                resultAction = ACTION_CHANNEL_SORT_ALL;
                break;
            case KeyEvent.KEYCODE_PROG_GREEN:
                resultAction = ACTION_CHANNEL_SORT_AZ;
                break;
            case KeyEvent.KEYCODE_PROG_YELLOW:
                resultAction = ACTION_CHANNEL_SORT_TP;
                break;
            case KeyEvent.KEYCODE_PROG_BLUE:
                resultAction = ACTION_CHANNEL_SORT_NETWORKID;
                break;
            case KeyEvent.KEYCODE_INFO:
                resultAction = ACTION_CHANNEL_SORT_SORT;
                break;
            case KeyEvent.KEYCODE_F1:
                resultAction = ACTION_FUNVTION_FIND;
                break;
            case KeyEvent.KEYCODE_F2:
                resultAction = ACTION_FUNVTION_ADD_FAV;
                break;
            case KeyEvent.KEYCODE_F3:
                resultAction = ACTION_FUNVTION_SATELLITE;
                break;
            case KeyEvent.KEYCODE_F4:
                resultAction = ACTION_FUNVTION_FAVLIST;
                break;
            default:
                break;
        }
        return resultAction;
    }

    private void dealAction(int action) {
        LOG(LOGD, null, "dealAction = " + action);
        mCurrentActionId = action;
        switch (action) {
            case ACTION_FUNVTION_FIND:
                mLeftTitle.setText(R.string.channel_list_all);
                if (mAllListView.getVisibility() != View.VISIBLE) {
                    mAllListView.setVisibility(View.VISIBLE);
                }
                if (mSortListView.getVisibility() == View.VISIBLE) {
                    mSortListView.setVisibility(View.GONE);
                }
                if (mContentListView.getVisibility() == View.VISIBLE) {
                    mContentListView.setVisibility(View.GONE);
                }
                if (mRightShowContainner.getVisibility() == View.VISIBLE) {
                    mRightShowContainner.setVisibility(View.GONE);
                }
                dealActionUI(action);
                break;
            case ACTION_CHANNEL_SORT_ALL:
            case ACTION_CHANNEL_SORT_AZ:
            case ACTION_CHANNEL_SORT_TP:
            case ACTION_CHANNEL_SORT_NETWORKID:
            case ACTION_FUNVTION_SATELLITE:
                mLeftTitle.setText(R.string.channel_list_all);
                if (mSortListView.getVisibility() != View.VISIBLE) {
                    mSortListView.setVisibility(View.VISIBLE);
                }
                if (mContentListView.getVisibility() != View.VISIBLE) {
                    mContentListView.setVisibility(View.VISIBLE);
                }
                if (mAllListView.getVisibility() == View.VISIBLE) {
                    mAllListView.setVisibility(View.GONE);
                }
                if (mRightShowContainner.getVisibility() == View.VISIBLE) {
                    mRightShowContainner.setVisibility(View.GONE);
                }
                dealActionUI(action);
                break;
            case ACTION_CHANNEL_SORT_SORT:
                if (mAllListView.getVisibility() == View.VISIBLE || mContentListView.getVisibility() == View.VISIBLE) {
                    dealActionUI(action);
                }
                break;
            case ACTION_FUNVTION_ADD_FAV:
                mFavListView.setListType(CustomedListView.SORT_FAV_LIST);
                mRightTitle.setText(R.string.f2_add_fav);
                if (mAllListView.getVisibility() == View.VISIBLE && !mAllListView.hasFocus() && mAllListView.getAdapter() != null && mAllListView.getAdapter().getCount() > 0) {
                    //Log.d(TAG, "dealAction --- " + mAllListView.getSelectedView() + ", position = " + mAllListView.getPositionForView(mAllListView.getSelectedView()));
                    updateEditChannelInfo(mAllListView, R.id.sort_channel_all, mAllListView.getPositionForView(mAllListView.getSelectedView()));
                } else if (mContentListView.getVisibility() == View.VISIBLE && !mContentListView.hasFocus() && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                    //Log.d(TAG, "dealAction --- " + mContentListView.getSelectedView() + ", item = " + mContentListView.getSelectedItem() + ", position = " + mContentListView.getPositionForView(mContentListView.getSelectedView()));
                    updateEditChannelInfo(mContentListView, R.id.sort_channel, mContentListView.getPositionForView(mContentListView.getSelectedView()));
                } else if (mAllListView.hasFocus() || mContentListView.hasFocus()) {
                    Log.d(TAG, "dealAction edit channel updated");
                } else {
                    mCurrentEditChannelList = null;
                    mCurrentEditChannelIndex = -1;
                    mCurrentEditChannelId = -1;
                    if (mRightShowContainner.getVisibility() == View.VISIBLE) {
                        mRightShowContainner.setVisibility(View.GONE);
                    }
                    Log.d(TAG, "dealAction ACTION_FUNVTION_ADD_FAV none selected edit channel");
                    return;
                }
                if (LIST_ALL_FAV_LIST.equals(mCurrentFavlList)) {
                    if (mRightShowContainner.getVisibility() != View.VISIBLE) {
                        mRightShowContainner.setVisibility(View.VISIBLE);
                    }
                } else {
                    if (mRightShowContainner.getVisibility() != View.VISIBLE) {
                        mRightShowContainner.setVisibility(View.VISIBLE);
                    } else {
                        mRightShowContainner.setVisibility(View.GONE);
                        return;
                    }
                }
                dealActionUI(action);
                break;
            case ACTION_FUNVTION_FAVLIST:
                mFavListView.setListType(CustomedListView.SORT_EDIT_ALL_FAV_LIST);
                mRightTitle.setText(R.string.favourite_list);
                if (LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList)) {
                    if (mRightShowContainner.getVisibility() != View.VISIBLE) {
                        mRightShowContainner.setVisibility(View.VISIBLE);
                    }
                } else {
                    if (mRightShowContainner.getVisibility() != View.VISIBLE) {
                        mRightShowContainner.setVisibility(View.VISIBLE);
                    } else {
                        mRightShowContainner.setVisibility(View.GONE);
                        return;
                    }
                }
                dealActionUI(action);
                break;
            default:
                break;
        }
    }

    private void dealLongPressAction(int action) {
        LOG(LOGD, null, "dealLongPressAction = " + action);
        switch (action) {
            case ACTION_FUNVTION_FAVLIST:
                mRightTitle.setText(R.string.edit_fav_list);
                /*if (mAllListView.getVisibility() == View.VISIBLE) {
                    mAllListView.setVisibility(View.GONE);
                }
                if (mSortListView.getVisibility() == View.VISIBLE) {
                    mSortListView.setVisibility(View.GONE);
                }
                if (mContentListView.getVisibility() == View.VISIBLE) {
                    mContentListView.setVisibility(View.GONE);
                }*/
                if (mRightShowContainner.getVisibility() != View.VISIBLE) {
                    mRightShowContainner.setVisibility(View.VISIBLE);
                }
                dealLongActionUI(action);
                break;
            default:
                break;
        }
    }

    private void dealLongActionUI(int action) {
        switch (action) {
            case ACTION_FUNVTION_FAVLIST:
                mFavListView.updateAllItem(this, mChannelDataManager.getEditFavListItem());
                mFavListView.requestFocus();
                mFavListView.setSelection(0);
                break;
            default:
                break;
        }
    }

    private void dealActionUI(int action) {
        LOG(LOGD, null, "dealActionUI = " + action);
        switch (action) {
            case ACTION_CHANNEL_SORT_ALL:{
                mLeftTitle.setText(R.string.channel_list_all);
                //mAllListView.updateAllItem(this, mChannelDataManager.getChannelListItem(mInputId));
                mSortListView.updateAllItem(this, mChannelDataManager.getChannelServiceTypeListItem(mInputId));
                mSortListView.setSelection(0);
                mSortListView.requestFocus();
                ItemAdapter adapter = (ItemAdapter) mSortListView.getAdapter();
                ChannelListItem item = null;
                String key = null;
                if (adapter != null && adapter.getCount() > 0) {
                    item = (ChannelListItem) adapter.getItem(0);
                }
                if (item != null) {
                    key = item.getKey();
                }
                mContentListView.updateAllItem(this, mChannelDataManager.getChannelListItemByType(mInputId, key));
                break;
            }
            case ACTION_CHANNEL_SORT_AZ:{
                mLeftTitle.setText(R.string.channel_list_all);
                mSortListView.updateAllItem(this, mChannelDataManager.getAZSortKeyChannelListItem(mInputId));
                mSortListView.setSelection(0);
                mSortListView.requestFocus();
                ItemAdapter adapter = (ItemAdapter) mSortListView.getAdapter();
                ChannelListItem item = null;
                String alphabet = null;
                if (adapter != null && adapter.getCount() > 0) {
                    item = (ChannelListItem) adapter.getItem(0);
                }
                if (item != null) {
                    alphabet = item.getKey();
                }
                mContentListView.updateAllItem(this, mChannelDataManager.getAZSortChannelListItemByStartedAlphabet(mInputId, alphabet));
                break;
            }
            case ACTION_CHANNEL_SORT_TP: {
                mLeftTitle.setText(R.string.channel_list_all);
                mSortListView.updateAllItem(this, mChannelDataManager.getTPSortKeyChannelListItem(mInputId));
                //mSortListView.updateAllItem(this, mChannelDataManager.getFrequencySortKeyChannelListItem(mInputId));
                mSortListView.setSelection(0);
                mSortListView.requestFocus();
                ItemAdapter adapter = (ItemAdapter) mSortListView.getAdapter();
                ChannelListItem item = null;
                String tpName = null;
                if (adapter != null && adapter.getCount() > 0) {
                    item = (ChannelListItem) adapter.getItem(0);
                }
                if (item != null) {
                    tpName = item.getKey();
                }
                mContentListView.updateAllItem(this, mChannelDataManager.getTPSortChannelListItemByName(mInputId, tpName));
                //mContentListView.updateAllItem(this, mChannelDataManager.getFrequencySortChannelListItemByFrequrncy(mInputId, Integer.valueOf(tpName)));
                break;
            }
            case ACTION_CHANNEL_SORT_NETWORKID: {
                mLeftTitle.setText(R.string.channel_list_all);
                mSortListView.updateAllItem(this, mChannelDataManager.getOperatorSortKeyChannelListItem(mInputId));
                mSortListView.setSelection(0);
                mSortListView.requestFocus();
                ItemAdapter adapter = (ItemAdapter) mSortListView.getAdapter();
                ChannelListItem item = null;
                int operatorId = 0;
                if (adapter != null && adapter.getCount() > 0) {
                    item = (ChannelListItem) adapter.getItem(0);
                }
                String key = item.getKey();
                if (item != null && !TextUtils.isEmpty(key)) {
                    operatorId = Integer.valueOf(item.getKey());
                }
                mContentListView.updateAllItem(this, mChannelDataManager.getOperatorSortChannelListItemByNetworkId(mInputId, operatorId));
                break;
            }
            case ACTION_CHANNEL_SORT_SORT:
                /*if (LIST_ALL_CHANNEL.equals(mCurrentEditChannelList)) {

                } else if (LIST_SORT_CONTENT.equals(mCurrentEditChannelList)) {

                }*/
                mCustomedDialogView.creatSortOrderDialog().show();
                break;
            case ACTION_FUNVTION_FIND: {
                mLeftTitle.setText(R.string.channel_search_result);
                mAllListView.updateAllItem(this, mChannelDataManager.getMatchedSortChannelListItemByName(mInputId, ""));
                mAllListView.setSelection(0);
                mAllListView.requestFocus();
                mCustomedDialogView.creatSeachChannelDialog().show();
                break;
            }
            case ACTION_FUNVTION_ADD_FAV: {
                if (mRightShowContainner.getVisibility() == View.VISIBLE && mCurrentEditChannelList != null && mCurrentEditChannelIndex > -1 && mCurrentEditChannelId > -1) {
                    ItemAdapter adapter = null;
                    if (TextUtils.equals(mCurrentEditChannelList, LIST_ALL_CHANNEL)) {
                        adapter = (ItemAdapter)mAllListView.getAdapter();
                    } else if (TextUtils.equals(mCurrentEditChannelList, LIST_SORT_CONTENT)) {
                        adapter = (ItemAdapter)mContentListView.getAdapter();
                    }
                    if (adapter != null && adapter.getCount() > 0 && adapter.getCount() > mCurrentEditChannelIndex) {
                        mFavListView.updateAllItem(this, mChannelDataManager.getChannelFavListItem((ChannelListItem)adapter.getItem(mCurrentEditChannelIndex)));
                        mFavListView.requestFocus();
                        mFavListView.setSelection(0);
                    }
                }
                break;
            }
            case ACTION_FUNVTION_SATELLITE: {
                mLeftTitle.setText(R.string.channel_list_all);
                mSortListView.updateAllItem(this, mChannelDataManager.getSatelliteSortKeyChannelListItem(mInputId));
                mSortListView.setSelection(0);
                mSortListView.requestFocus();
                ItemAdapter adapter = (ItemAdapter) mSortListView.getAdapter();
                ChannelListItem item = null;
                String satellite = null;
                if (adapter != null && adapter.getCount() > 0) {
                    item = (ChannelListItem) adapter.getItem(0);
                }
                if (item != null) {
                    satellite = item.getKey();
                }
                mContentListView.updateAllItem(this, mChannelDataManager.getSatelliteSortChannelListItemByName(mInputId, satellite));
                break;
            }
            case ACTION_FUNVTION_FAVLIST: {
                mFavListView.updateAllItem(this, /*mChannelDataManager.getFavListItem()*/mChannelDataManager.getEditFavListItem());
                mFavListView.requestFocus();
                mFavListView.setSelection(0);
                break;
            }
            default:
                break;

        }
    }

    /*private void init() {
        Button start = findViewById(R.id.start);
        Button stop = findViewById(R.id.stop);
        Button bind = findViewById(R.id.bind);
        Button unbind = findViewById(R.id.unbind);
        start.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startInterationService();
            }
        });
        stop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopInterationService();
            }
        });
        bind.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                bindInterationService();
            }
        });
        unbind.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                unbindInterationService();
            }
        });
    }*/

    private void init() {
        mLeftTitle = (TextView)findViewById(R.id.channellist_title);
        mRightTitle = (TextView)findViewById(R.id.favlist_title);
        mAllButton = (Button)findViewById(R.id.red_sort_button);
        mATwoZButton = (Button)findViewById(R.id.green_sort_button);
        mTPButton = (Button)findViewById(R.id.yellow_sort_button);
        mNetWorkIdButton = (Button)findViewById(R.id.blue_sort_button);
        mExtraSortButton = (Button)findViewById(R.id.sort_sort_button);
        mSatelliteText = (TextView)findViewById(R.id.sat_text);
        mSatellite = (TextView)findViewById(R.id.sat);
        mTransponder = (TextView)findViewById(R.id.trans);

        mFindButton = (Button)findViewById(R.id.f1_button);
        mAddToFavButton = (Button)findViewById(R.id.f2_button);
        mSatelliteButton = (Button)findViewById(R.id.f3_button);
        mFavListButton = (Button)findViewById(R.id.f4_button);

        mSortListView = (ChannelListListView) findViewById(R.id.sort_key);
        mContentListView = (ChannelListListView)findViewById(R.id.sort_channel);
        mAllListView = (ChannelListListView)findViewById(R.id.sort_channel_all);
        mFavListView = (FavListListView) findViewById(R.id.favourite);
        mRightShowContainner = (LinearLayout) findViewById(R.id.right_show);
        mCustomedDialogView = new CustomedDialogView(this, mDialogCallback);

        setTvandRadioAsDefault();
        setListener();
    }

    private void setTvandRadioAsDefault() {
        //statrt init all video and radio
        mAllListView.setVisibility(View.GONE);
        mSortListView.setVisibility(View.VISIBLE);
        mContentListView.setVisibility(View.VISIBLE);
        mRightShowContainner.setVisibility(View.GONE);
        mLeftTitle.setText(R.string.channel_list_all);
        mSortListView.updateAllItem(this, mChannelDataManager.getChannelServiceTypeListItem(mInputId));
        mSortListView.setSelection(0);
        mSortListView.requestFocus();
        ItemAdapter adapter = (ItemAdapter) mSortListView.getAdapter();
        ChannelListItem item = null;
        String key = null;
        if (adapter != null && adapter.getCount() > 0) {
            item = (ChannelListItem) adapter.getItem(0);
        }
        if (item != null) {
            key = item.getKey();
        }
        mContentListView.updateAllItem(this, mChannelDataManager.getChannelListItemByType(mInputId, key));
    }

    private void setListener() {
        mAllListView.setOnItemClickListener(mOnItemClickListener);
        mAllListView.setOnItemSelectedListener(mOnItemSelectedListener);
        mAllListView.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mAllListView.setListType(CustomedListView.SORT_ALL_LIST);
        mAllListView.setKeyEventListener(mKeyEventListener);

        mSortListView.setOnItemClickListener(mOnItemClickListener);
        mSortListView.setOnItemSelectedListener(mOnItemSelectedListener);
        mSortListView.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mSortListView.setListType(CustomedListView.SORT_KEY_LIST);
        mSortListView.setKeyEventListener(mKeyEventListener);

        mContentListView.setOnItemClickListener(mOnItemClickListener);
        mContentListView.setOnItemSelectedListener(mOnItemSelectedListener);
        mContentListView.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mContentListView.setListType(CustomedListView.SORT_KEY_CONTENT_LIST);
        mContentListView.setKeyEventListener(mKeyEventListener);

        mFavListView.setOnItemClickListener(mOnItemClickListener);
        mFavListView.setOnItemSelectedListener(mOnItemSelectedListener);
        mFavListView.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mFavListView.setListType(CustomedListView.SORT_FAV_LIST);
        mFavListView.setKeyEventListener(mKeyEventListener);

        mAllButton.setOnClickListener(mOnClickListener);
        mAllButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mATwoZButton.setOnClickListener(mOnClickListener);
        mATwoZButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mTPButton.setOnClickListener(mOnClickListener);
        mTPButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mNetWorkIdButton.setOnClickListener(mOnClickListener);
        mNetWorkIdButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mExtraSortButton = (Button)findViewById(R.id.sort_sort_button);
        mExtraSortButton.setOnClickListener(mOnClickListener);
        mExtraSortButton.setOnFocusChangeListener(mOnItemFocusChangeListener);

        mFindButton.setOnClickListener(mOnClickListener);
        mFindButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mAddToFavButton.setOnClickListener(mOnClickListener);
        mAddToFavButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mSatelliteButton.setOnClickListener(mOnClickListener);
        mSatelliteButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mFavListButton.setOnClickListener(mOnClickListener);
        mFavListButton.setOnFocusChangeListener(mOnItemFocusChangeListener);
        mFavListButton.setOnLongClickListener(mOnLongClickListener);
    }

    private void LOG(String type, String extra, String value) {
        if (DEBUG && LOGD.equals(type)) {
            if (TextUtils.isEmpty(extra)) {
                Log.d(TAG, value);
            } else {
                Log.d(extra, value);
            }
        } else if (LOGE.equals(type)) {
            if (TextUtils.isEmpty(extra)) {
                Log.e(TAG, value);
            } else {
                Log.e(extra, value);
            }
        }
    }

    private static final String LIST_ALL_CHANNEL = "all_channel_list";
    private static final String LIST_SORT_KEY = "sort_key_list";
    private static final String LIST_SORT_CONTENT = "sort_content_list";
    private static final String LIST_ALL_FAV_LIST = "all_fav_list";
    private static final String LIST_CHANNEL_FAV_LIST = "all_channel_fav_list";
    private static final String LIST_EDIT_ALL_FAV_LIST = "edit_fav_list";
    private String mCurrentEditChannelList =  LIST_ALL_CHANNEL;//content or all channel
    private long mCurrentEditChannelId =  -1;
    private int mCurrentEditChannelIndex =  -1;
    private String mCurrentFavlList =  LIST_ALL_FAV_LIST;//all or channel fav
    //private int mCurrentFavId = -1;
    private String mCurrentFavName = null;
    private int mCurrentFavIndex =  -1;
    private int mCurrentActionId =  ACTION_CHANNEL_SORT_ALL;

    private AdapterView.OnItemSelectedListener mOnItemSelectedListener = new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            if (DEBUG) {
                LOG(LOGD, null, "onItemSelected view = " + view + ", position = " + position);
            }
            if (parent != null) {
                int parentRes = parent.getId();
                switch (parentRes) {
                    case R.id.sort_key:
                        LOG(LOGD, null, "onItemSelected sort_key position = " + position + ", id = " + id);
                        updateSortKeyChange(parent, parentRes, position);
                        break;
                    case R.id.sort_channel:
                        LOG(LOGD, null, "onItemSelected sort_channel position = " + position + ", id = " + id);
                        updateEditChannelInfo(parent, parentRes, position);
                        break;
                    case R.id.sort_channel_all:
                        LOG(LOGD, null, "onItemSelected sort_channel_all position = " + position + ", id = " + id + ", view = " + view);
                        updateEditChannelInfo(parent, parentRes, position);
                        break;
                    case R.id.favourite:
                        LOG(LOGD, null, "onItemSelected favourite position = " + position + ", id = " + id);
                        FavListItem item = null;
                        if (parent instanceof FavListListView) {
                            Object obj = parent.getItemAtPosition(position);
                            if (obj instanceof FavListItem) {
                                item = (FavListItem)obj;
                            }
                        }
                        if (item != null) {
                            if (LIST_ALL_FAV_LIST.equals(item.getFavListType())) {
                                mCurrentFavlList = LIST_ALL_FAV_LIST;
                                //mCurrentFavId = item.getFavId();
                                mCurrentFavName = item.getTitle();
                                mCurrentFavIndex = position;
                            } else if (LIST_CHANNEL_FAV_LIST.equals(item.getFavListType())){
                                mCurrentFavlList = LIST_CHANNEL_FAV_LIST;
                                //mCurrentFavId = item.getFavId();
                                mCurrentFavName = item.getTitle();
                                mCurrentFavIndex = position;
                            } else if (LIST_EDIT_ALL_FAV_LIST.equals(item.getFavListType())) {
                                mCurrentFavlList = LIST_EDIT_ALL_FAV_LIST;
                                //mCurrentFavId = item.getFavId();
                                mCurrentFavName = item.getTitle();
                                mCurrentFavIndex = position;
                            }
                            LOG(LOGD, null, "onItemSelected favourite mCurrentFavlList = " + mCurrentFavlList + ", mCurrentFavName = " + mCurrentFavName + ", mCurrentFavIndex = " + mCurrentFavIndex);
                        }
                        break;
                    default:
                        LOG(LOGD, null, "onItemSelected parent = " + parent + ", view = " + view + ", position = " + position + ", id = " + id);
                        break;
                }
            } else {
                LOG(LOGD, null, "onItemSelected parent null");
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            LOG(LOGD, null, "onNothingSelected parent = " + parent);
        }
    };

    private AdapterView.OnItemClickListener mOnItemClickListener = new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if (DEBUG) {
                LOG(LOGD, null, "onItemClick view = " + view + ", position = " + position);
            }
            if (parent != null) {
                int parentRes = parent.getId();
                switch (parentRes) {
                    case R.id.sort_key:
                        LOG(LOGD, null, "onItemClick sort_key position = " + position + ", id = " + id);
                        break;
                    case R.id.sort_channel:
                        LOG(LOGD, null, "onItemClick sort_channel position = " + position + ", id = " + id);
                        updateEditChannelInfo(parent, parentRes, position);
                        switchChannelWhenClick(parent, parentRes, position);
                        break;
                    case R.id.sort_channel_all:
                        LOG(LOGD, null, "onItemClick sort_channel position = " + position + ", id = " + id + "，　view = " + view);
                        updateEditChannelInfo(parent, parentRes, position);
                        switchChannelWhenClick(parent, parentRes, position);
                        break;
                    case R.id.favourite:
                        LOG(LOGD, null, "onItemClick favourite position = " + position + ", id = " + id);
                        FavListItem favItem = null;
                        if (parent instanceof FavListListView) {
                            favItem = (FavListItem)(parent.getItemAtPosition(position));
                        }
                        if (favItem != null) {
                            if (LIST_ALL_FAV_LIST.equals(favItem.getFavListType())) {
                                mCurrentFavlList = LIST_ALL_FAV_LIST;
                                //mCurrentFavId = favItem.getFavId();
                                mCurrentFavName = favItem.getTitle();
                                mCurrentFavIndex = position;
                            } else if (LIST_CHANNEL_FAV_LIST.equals(favItem.getFavListType())){
                                mCurrentFavlList = LIST_CHANNEL_FAV_LIST;
                                //mCurrentFavId = favItem.getFavId();
                                mCurrentFavName = favItem.getTitle();
                                mCurrentFavIndex = position;
                            } else if (LIST_EDIT_ALL_FAV_LIST.equals(favItem.getFavListType())) {
                                mCurrentFavlList = LIST_EDIT_ALL_FAV_LIST;
                                //mCurrentFavId = favItem.getFavId();
                                mCurrentFavName = favItem.getTitle();
                                mCurrentFavIndex = position;
                            }
                            LOG(LOGD, null, "OnItemClickListener favourite mCurrentFavlList = " + mCurrentFavlList + ", mCurrentFavName = " + mCurrentFavName + ", mCurrentFavIndex = " + mCurrentFavIndex);
                        }
                        updateChannelFavInfo(parent, parentRes, position);
                        break;
                    default:
                        LOG(LOGD, null, "onItemClick parent = " + parent + ", view = " + view + ", position = " + position + ", id = " + id);
                        break;
                }
            } else {
                LOG(LOGD, null, "onItemClick parent null");
            }
        }
    };

    private CustomedListView.KeyEventListener mKeyEventListener = new CustomedListView.KeyEventListener() {
        @Override
        public void onKeyEventCallbak(Item item, Bundle data) {
            if (DEBUG) {
                LOG(LOGD, null, "onKeyEventCallbak item = " + item + ", data = " + data);
            }
            if (data != null) {
                String listType = data.getString(CustomedListView.KEY_LIST_TYPE);
                switch (listType) {
                    case CustomedListView.SORT_ALL_LIST:
                        if (KeyEvent.KEYCODE_DPAD_LEFT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {

                        } else if (KeyEvent.KEYCODE_DPAD_RIGHT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {
                            if (mFavListView.getVisibility() == View.VISIBLE && mFavListView.getAdapter() != null && mFavListView.getAdapter().getCount() > 0) {
                                mFavListView.requestFocus();
                            }
                        }
                        break;
                    case CustomedListView.SORT_KEY_LIST:
                        if (KeyEvent.KEYCODE_DPAD_LEFT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {

                        } else if (KeyEvent.KEYCODE_DPAD_RIGHT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {
                            if (mContentListView.getVisibility() == View.VISIBLE && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                mContentListView.requestFocus();
                            }
                        }
                        break;
                    case CustomedListView.SORT_KEY_CONTENT_LIST:
                        if (KeyEvent.KEYCODE_DPAD_LEFT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {
                            if (mSortListView.getVisibility() == View.VISIBLE && mSortListView.getAdapter() != null && mSortListView.getAdapter().getCount() > 0) {
                                mSortListView.requestFocus();
                            }
                        } else if (KeyEvent.KEYCODE_DPAD_RIGHT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {
                            if (mFavListView.getVisibility() == View.VISIBLE && mFavListView.getAdapter() != null && mFavListView.getAdapter().getCount() > 0) {
                                mFavListView.requestFocus();
                            }
                        }
                        break;
                    case CustomedListView.SORT_FAV_LIST:
                    case CustomedListView.SORT_EDIT_ALL_FAV_LIST:
                        if (KeyEvent.KEYCODE_DPAD_LEFT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {
                            if (mAllListView.getVisibility() == View.VISIBLE && mAllListView.getAdapter() != null && mAllListView.getAdapter().getCount() > 0) {
                                mAllListView.requestFocus();
                            } else if (mContentListView.getVisibility() == View.VISIBLE && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                mContentListView.requestFocus();
                            } else if (mSortListView.getVisibility() == View.VISIBLE && mSortListView.getAdapter() != null && mSortListView.getAdapter().getCount() > 0) {
                                mSortListView.requestFocus();
                            }
                        } else if (KeyEvent.KEYCODE_DPAD_RIGHT == data.getInt(CustomedListView.KEY_ACTION_CODE)) {
                            if (TextUtils.equals(listType, CustomedListView.SORT_EDIT_ALL_FAV_LIST) && item !=  null && item instanceof FavListItem) {
                                if (mRightShowContainner.getVisibility() == View.VISIBLE) {
                                    //display current page
                                    if (mCurrentFavName != null) {
                                        if (mSortListView.getVisibility() == View.VISIBLE) {
                                            mSortListView.setVisibility(View.GONE);
                                        }
                                        if (mContentListView.getVisibility() == View.VISIBLE) {
                                            mContentListView.setVisibility(View.GONE);
                                        }
                                        if (mAllListView.getVisibility() != View.VISIBLE) {
                                            mAllListView.setVisibility(View.VISIBLE);
                                        }
                                        mLeftTitle.setText(mCurrentFavName);
                                        mAllListView.updateAllItem(SortFavActivity.this, mChannelDataManager.getChannelItemByFavPage(mCurrentFavName));
                                    }
                                    mCustomedDialogView.creatEditFavDialog((FavListItem)item).show();
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    };

    private AdapterView.OnFocusChangeListener mOnItemFocusChangeListener = new AdapterView.OnFocusChangeListener() {
        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            /*if (v != null) {
                int listViewRes = v.getId();
                switch (listViewRes) {
                    case R.id.sort_key:
                        LOG(LOGD, null, "onFocusChange sort_key hasFocus = " + hasFocus);
                        if (hasFocus) {
                            //mSortListView.setSelection(mSortListView.getSelectedItemPosition());
                        }
                        break;
                    case R.id.sort_channel:
                        LOG(LOGD, null, "onFocusChange sort_channel hasFocus = " + hasFocus);
                        if (hasFocus) {
                            //mContentListView.setSelection(mContentListView.getSelectedItemPosition());
                        }
                        break;
                    case R.id.sort_channel_all:
                        LOG(LOGD, null, "onFocusChange sort_channel_all hasFocus = " + hasFocus);
                        if (hasFocus) {
                            //mAllListView.setSelection(mAllListView.getSelectedItemPosition());
                        }
                        break;
                    case R.id.favourite:
                        LOG(LOGD, null, "onFocusChange favourite hasFocus = " + hasFocus);
                        if (hasFocus) {
                            //mFavListView.setSelection(mFavListView.getSelectedItemPosition());
                        }
                        break;
                    case R.id.red_sort_button:
                        LOG(LOGD, null, "onFocusChange red_sort_button hasFocus = " + hasFocus);

                        break;
                    case R.id.green_sort_button:
                        LOG(LOGD, null, "onFocusChange green_sort_button hasFocus = " + hasFocus);

                        break;
                    case R.id.yellow_sort_button:
                        LOG(LOGD, null, "onFocusChange yellow_sort_button hasFocus = " + hasFocus);

                        break;
                    case R.id.blue_sort_button:
                        LOG(LOGD, null, "onFocusChange blue_sort_button hasFocus = " + hasFocus);

                        break;
                    case R.id.sort_sort_button:
                        LOG(LOGD, null, "onFocusChange sort_sort_button hasFocus = " + hasFocus);

                        break;
                    case R.id.f1_button:
                        LOG(LOGD, null, "onFocusChange f1_button hasFocus = " + hasFocus);

                        break;
                    case R.id.f2_button:
                        LOG(LOGD, null, "onFocusChange f2_button hasFocus = " + hasFocus);

                        break;
                    case R.id.f3_button:
                        LOG(LOGD, null, "onFocusChange f3_button hasFocus = " + hasFocus);

                        break;
                    case R.id.f4_button:
                        LOG(LOGD, null, "onFocusChange f4_button hasFocus = " + hasFocus);

                        break;
                    default:
                        LOG(LOGD, null, "onFocusChange other = " + v + ", hasFocus = " + hasFocus);
                        break;
                }
            } else {
                LOG(LOGD, null, "onFocusChange v null");
            }*/
        }
    };

    private View.OnClickListener mOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (v != null) {
                int buttonRes = v.getId();
                dealAction(getButtonClickAction(buttonRes));
            }
        }
    };

    private View.OnLongClickListener mOnLongClickListener = new View.OnLongClickListener() {
        @Override
        public boolean onLongClick(View v) {
            if (v != null) {
                int buttonRes = v.getId();
                dealLongActionUI(getButtonClickAction(buttonRes));
            }
            return true;
        }
    };

    private CustomedDialogView.DialogCallback mDialogCallback = new CustomedDialogView.DialogCallback() {
        @Override
        public void onDialogEvent(Bundle bundle) {
            if (DEBUG) {
                LOG(LOGD, null, "onDialogEvent bundle = " + bundle);
            }
            if (bundle != null) {
                if (bundle.getBoolean(CustomedDialogView.DIALOG_ACTION)) {
                    if (CustomedDialogView.DIALOG_ACTION_SORT_LIST_CLICKED.equals(bundle.getString(CustomedDialogView.DIALOG_EVENT))) {
                        int flag = bundle.getInt(CustomedDialogView.DIALOG_LIST_POSITION, -1);
                        if (flag > -1) {
                            if (mAllListView.getVisibility() == View.VISIBLE && mAllListView.getAdapter() != null && mAllListView.getAdapter().getCount() > 0) {
                                LinkedList<Item> allData = ((ItemAdapter)mAllListView.getAdapter()).getAllData();
                                //int position = mAllListView.getPositionForView(mAllListView.getSelectedView());
                                switch (flag) {
                                    case CustomedDialogView.FLAG_SORT_ITEM_AZ:
                                        allData = mChannelDataManager.getAZSortedItemList(allData);
                                        break;
                                    case CustomedDialogView.FLAG_SORT_ITEM_ZA:
                                        allData = mChannelDataManager.getZASortedItemList(allData);
                                        break;
                                    default:
                                        break;
                                }
                                mAllListView.updateAllItem(SortFavActivity.this, allData);
                                //mAllListView.setSelection(position);
                            } else if (mContentListView.getVisibility() == View.VISIBLE && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                LinkedList<Item> allData = ((ItemAdapter)mContentListView.getAdapter()).getAllData();
                                switch (flag) {
                                    case CustomedDialogView.FLAG_SORT_ITEM_AZ:
                                        allData = mChannelDataManager.getAZSortedItemList(allData);
                                        break;
                                    case CustomedDialogView.FLAG_SORT_ITEM_ZA:
                                        allData = mChannelDataManager.getZASortedItemList(allData);
                                        break;
                                    default:
                                        break;
                                }
                                mContentListView.updateAllItem(SortFavActivity.this, allData);
                                //mContentListView.setSelection(0);
                            }
                        }
                    } else if (CustomedDialogView.DIALOG_ACTION_SEARCH_BUTTON_CLICKED.equals(bundle.getString(CustomedDialogView.DIALOG_EVENT)) ||
                            CustomedDialogView.DIALOG_ACTION_SEARCH_CONTENT_CHANGED.equals(bundle.getString(CustomedDialogView.DIALOG_EVENT))) {
                        String matchKey = bundle.getString(CustomedDialogView.DIALOG_ACTION_SEARCH_VALUE);
                        if (mAllListView.getVisibility() == View.VISIBLE) {
                            mAllListView.updateAllItem(SortFavActivity.this, mChannelDataManager.getMatchedSortChannelListItemByName(mInputId, matchKey));
                            mAllListView.setSelection(0);
                        }
                    } else if (CustomedDialogView.DIALOG_ACTION_EDIT_FAV_OK_CLICKED.equals(bundle.getString(CustomedDialogView.DIALOG_EVENT))) {
                        if (mFavListView.getVisibility() == View.VISIBLE) {
                            mHandler.sendMessage(mHandler.obtainMessage(UPDATE_FAV_GROUP, bundle));
                        }
                    }
                }
            }
        }
    };

    //update by select channel fav
    private void updateChannelFavInfo(AdapterView<?> parent, int resId, int position) {
        if (parent != null && parent instanceof FavListListView) {
            FavListItem favItem = null;
            ItemAdapter favAdapter = null;
            String favType = null;
            boolean isSelectedFav = false;
            //int favId = -1;
            String favTitle = null;
            favItem = (FavListItem)parent.getItemAtPosition(position);
            LinkedList<Item> data =  ((ItemAdapter)parent.getAdapter()).getAllData();
            if (favItem != null) {
                favType = favItem.getFavListType();
                //favId = favItem.getFavId();
                favTitle = favItem.getTitle();
                isSelectedFav = favItem.isNeedShowIcon();
                LOG(LOGD, null, "updateChannelFavInfo favType = " + favType + ", favTitle = " + favTitle + ", isSelectedFav = " + isSelectedFav);
                if (LIST_ALL_FAV_LIST.equals(favItem.getFavListType())) {
                    mCurrentFavlList = LIST_ALL_FAV_LIST;
                    //mCurrentFavId = favItem.getFavId();
                    mCurrentFavName = favItem.getTitle();
                    mCurrentFavIndex = position;
                } else if (LIST_CHANNEL_FAV_LIST.equals(favItem.getFavListType())){
                    mCurrentFavlList = LIST_CHANNEL_FAV_LIST;
                    //mCurrentFavId = favItem.getFavId();
                    mCurrentFavName = favItem.getTitle();
                    mCurrentFavIndex = position;
                } else if (LIST_EDIT_ALL_FAV_LIST.equals(favItem.getFavListType())) {
                    mCurrentFavlList = LIST_EDIT_ALL_FAV_LIST;
                    //mCurrentFavId = favItem.getFavId();
                    mCurrentFavName = favItem.getTitle();
                    mCurrentFavIndex = position;
                }
                if (LIST_CHANNEL_FAV_LIST.equals(favType)) {
                    isSelectedFav = !isSelectedFav;
                    favItem.setNeedShowIcon(isSelectedFav);
                    mFavListView.updateItem(position, favItem);
                    if (TextUtils.equals(mCurrentEditChannelList, LIST_ALL_CHANNEL)) {
                        ItemAdapter adapter = (ItemAdapter)(mAllListView.getAdapter());
                        ChannelListItem channelItem = (ChannelListItem)(adapter.getItem(mCurrentEditChannelIndex));
                        channelItem.getUpdateFavAllIndexArrayString(isSelectedFav, favTitle);
                        mAllListView.updateItem(mCurrentEditChannelIndex, channelItem);
                        //adapter = (ItemAdapter)(mAllListView.getAdapter());
                        //String updateChannel = mChannelDataManager.genarateUpdatedChannelListJsonSrt(mChannelDataManager.getChannelList(mInputId), channelItem.getChannelId(), channelItem.getFavArrayJsonStr());
                        //mChannelDataManager.updateChannelListChangeToDatabase(updateChannel);
                        mChannelDataManager.updateChannelFavChangeToTvProvider(channelItem.getChannelId(), channelItem.getFavArrayJsonStr());
                    } else if (TextUtils.equals(mCurrentEditChannelList, LIST_SORT_CONTENT)) {
                        ItemAdapter adapter = (ItemAdapter)(mContentListView.getAdapter());
                        ChannelListItem channelItem = (ChannelListItem)(adapter.getItem(mCurrentEditChannelIndex));
                        channelItem.getUpdateFavAllIndexArrayString(isSelectedFav, favTitle);
                        mContentListView.updateItem(mCurrentEditChannelIndex, channelItem);
                        //String updateChannel = mChannelDataManager.genarateUpdatedChannelListJsonSrt(mChannelDataManager.getChannelList(mInputId), channelItem.getChannelId(), channelItem.getFavArrayJsonStr());
                        //mChannelDataManager.updateChannelListChangeToDatabase(updateChannel);
                        mChannelDataManager.updateChannelFavChangeToTvProvider(channelItem.getChannelId(), channelItem.getFavArrayJsonStr());
                    }
                } else if (LIST_ALL_FAV_LIST.equals(favType) ||
                        LIST_EDIT_ALL_FAV_LIST.equals(favType)) {
                    if (mSortListView.getVisibility() == View.VISIBLE) {
                        mSortListView.setVisibility(View.GONE);
                    }
                    if (mContentListView.getVisibility() == View.VISIBLE) {
                        mContentListView.setVisibility(View.GONE);
                    }
                    if (mAllListView.getVisibility() != View.VISIBLE) {
                        mAllListView.setVisibility(View.VISIBLE);
                    }
                    mLeftTitle.setText(favItem.getTitle());
                    mAllListView.updateAllItem(this, mChannelDataManager.getChannelItemByFavPage(favItem.getTitle()));
                }/* else if (LIST_EDIT_ALL_FAV_LIST.equals(favType)) {
                    mCustomedDialogView.creatEditFavDialog(favItem).show();
                }*/
            }
        }
    }

    //update when channel item is focused
    private void updateEditChannelInfo(AdapterView<?> parent, int resId, int position) {
        switch (resId) {
            case R.id.sort_channel:
                mCurrentEditChannelList = LIST_SORT_CONTENT;
                break;
            case R.id.sort_channel_all:
                mCurrentEditChannelList = LIST_ALL_CHANNEL;
                break;
            default:
                mCurrentEditChannelList = null;
                mCurrentEditChannelIndex = -1;
                mCurrentEditChannelId = -1;
                return;
        }
        mCurrentEditChannelIndex = position;
        if (parent != null && parent instanceof ChannelListListView) {
            ChannelListItem item = (ChannelListItem)parent.getItemAtPosition(position);
            mCurrentEditChannelId = item.getChannelId();
            String satellitename = item.getSatellite();
            if (TextUtils.isEmpty(satellitename)) {
                mSatelliteText.setVisibility(View.GONE);
                satellitename = item.getChannelType();
            } else {
                mSatelliteText.setVisibility(View.VISIBLE);
            }
            mSatellite.setText(satellitename);
            mTransponder.setText(item.getTranponder());
            if (mRightShowContainner.getVisibility() == View.VISIBLE && mCurrentEditChannelList != null && mCurrentEditChannelIndex > -1 && mCurrentEditChannelId > -1) {
                ItemAdapter adapter = null;
                if (TextUtils.equals(mCurrentEditChannelList, LIST_ALL_CHANNEL)) {
                    adapter = (ItemAdapter)mAllListView.getAdapter();
                } else if (TextUtils.equals(mCurrentEditChannelList, LIST_SORT_CONTENT)) {
                    adapter = (ItemAdapter)mContentListView.getAdapter();
                }
                if (LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList) && adapter != null && adapter.getCount() > 0 && adapter.getCount() > mCurrentEditChannelIndex) {
                    mFavListView.updateAllItem(this, mChannelDataManager.getChannelFavListItem((ChannelListItem)adapter.getItem(mCurrentEditChannelIndex)));
                    //mFavListView.requestFocus();
                    mFavListView.setSelection(0);
                }
            }
        }
        LOG(LOGD, null, ("updateEditChannelInfo mCurrentEditChannelList = " + mCurrentEditChannelList + ", mCurrentEditChannelIndex = " + mCurrentEditChannelIndex + ", mCurrentEditChannelId =" + mCurrentEditChannelId));
    }

    private void switchChannelWhenClick(AdapterView<?> parent, int resId, int position) {
        if (parent != null && parent instanceof ChannelListListView) {
            ChannelListItem item = (ChannelListItem)parent.getItemAtPosition(position);
            long channelId = item.getChannelId();
            sendSwitchChannelBroadcast(channelId);
        }
    }

    //update when sort key list changed
    private void updateSortKeyChange(AdapterView<?> parent, int resId, int position) {
        if (parent instanceof ChannelListListView) {
            ChannelListItem sortKeyItem = null;
            ItemAdapter sortKeyAdapter = null;
            sortKeyItem = (ChannelListItem)parent.getItemAtPosition(position);
            int sortKeyType;
            String key =  null;
            if (sortKeyItem != null) {
                sortKeyType = sortKeyItem.getItemType();
                key = sortKeyItem.getKey();
                switch (sortKeyType) {
                    case ACTION_CHANNEL_SORT_ALL:
                        mContentListView.updateAllItem(this, mChannelDataManager.getChannelListItemByType(mInputId, key));
                        mContentListView.setSelection(0);
                        if (mRightShowContainner.getVisibility() == View.VISIBLE && LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList)) {
                            if (mContentListView.getVisibility() == View.VISIBLE && !mContentListView.hasFocus() && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                updateEditChannelInfo(mContentListView, R.id.sort_channel, mContentListView.getPositionForView(mContentListView.getSelectedView()));
                            }
                        }
                        break;
                    case ACTION_CHANNEL_SORT_AZ:
                        mContentListView.updateAllItem(this, mChannelDataManager.getAZSortChannelListItemByStartedAlphabet(mInputId, key));
                        mContentListView.setSelection(0);
                        if (mRightShowContainner.getVisibility() == View.VISIBLE && LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList)) {
                            if (mContentListView.getVisibility() == View.VISIBLE && !mContentListView.hasFocus() && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                updateEditChannelInfo(mContentListView, R.id.sort_channel, mContentListView.getPositionForView(mContentListView.getSelectedView()));
                            }
                        }
                        break;
                    case ACTION_CHANNEL_SORT_TP:
                        mContentListView.updateAllItem(this, mChannelDataManager.getTPSortChannelListItemByName(mInputId, key));
                        mContentListView.setSelection(0);
                        if (mRightShowContainner.getVisibility() == View.VISIBLE && LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList)) {
                            if (mContentListView.getVisibility() == View.VISIBLE && !mContentListView.hasFocus() && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                updateEditChannelInfo(mContentListView, R.id.sort_channel, mContentListView.getPositionForView(mContentListView.getSelectedView()));
                            }
                        }
                        break;
                    case ACTION_CHANNEL_SORT_NETWORKID:
                        mContentListView.updateAllItem(this, mChannelDataManager.getOperatorSortChannelListItemByNetworkId(mInputId, Integer.valueOf(key)));
                        mContentListView.setSelection(0);
                        if (mRightShowContainner.getVisibility() == View.VISIBLE && LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList)) {
                            if (mContentListView.getVisibility() == View.VISIBLE && !mContentListView.hasFocus() && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                updateEditChannelInfo(mContentListView, R.id.sort_channel, mContentListView.getPositionForView(mContentListView.getSelectedView()));
                            }
                        }
                        break;
                    case ACTION_FUNVTION_SATELLITE:
                        mContentListView.updateAllItem(this, mChannelDataManager.getSatelliteSortChannelListItemByName(mInputId, key));
                        mContentListView.setSelection(0);
                        if (mRightShowContainner.getVisibility() == View.VISIBLE && LIST_CHANNEL_FAV_LIST.equals(mCurrentFavlList)) {
                            if (mContentListView.getVisibility() == View.VISIBLE && !mContentListView.hasFocus() && mContentListView.getAdapter() != null && mContentListView.getAdapter().getCount() > 0) {
                                updateEditChannelInfo(mContentListView, R.id.sort_channel, mContentListView.getPositionForView(mContentListView.getSelectedView()));
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    private int getButtonClickAction(int buttonRes) {
        int resultAction = -1;
        switch (buttonRes) {
            case R.id.red_sort_button:
                resultAction = ACTION_CHANNEL_SORT_ALL;
                break;
            case R.id.green_sort_button:
                resultAction = ACTION_CHANNEL_SORT_AZ;
                break;
            case R.id.yellow_sort_button:
                resultAction = ACTION_CHANNEL_SORT_TP;
                break;
            case R.id.blue_sort_button:
                resultAction = ACTION_CHANNEL_SORT_NETWORKID;
                break;
            case R.id.sort_sort_button:
                resultAction = ACTION_CHANNEL_SORT_SORT;
                break;
            case R.id.f1_button:
                resultAction = ACTION_FUNVTION_FIND;
                break;
            case R.id.f2_button:
                resultAction = ACTION_FUNVTION_ADD_FAV;
                break;
            case R.id.f3_button:
                resultAction = ACTION_FUNVTION_SATELLITE;
                break;
            case R.id.f4_button:
                resultAction = ACTION_FUNVTION_FAVLIST;
        }
        return resultAction;
    }

    private void sendSwitchChannelBroadcast(long channelId) {
        if (channelId == -1) {
            return;
        }
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_SWITCH_CHANNEL);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, channelId);
        sendBroadcast(intent);
    }

    private void dealFavGroupChange(Bundle bundle) {
        int actionFlag = bundle.getInt(CustomedDialogView.DIALOG_ACTION_FLAG);
        String previousName = bundle.getString(CustomedDialogView.DIALOG_ACTION_FAV_PREVIOUS_VALUE);
        String newName = bundle.getString(CustomedDialogView.DIALOG_ACTION_FAV_EDIT_VALUE);
        ItemAdapter adapter = (ItemAdapter)mContentListView.getAdapter();
        String newFavDisplay = null;
        int position = 0;
        switch (actionFlag) {
            case CustomedDialogView.FLAG_EDIT_FAV_ADD:
                mChannelDataManager.addFavGroup(newName);
                newFavDisplay = newName;
                position = mChannelDataManager.getFavGroupPageIndex(newFavDisplay);
                break;
            case CustomedDialogView.FLAG_EDIT_FAV_DEL:
                mChannelDataManager.removeFavGroup(previousName);
                position = mChannelDataManager.getFavGroupPageIndex(previousName);
                if (position > 0) {
                    position = position - 1;
                }
                break;
            case CustomedDialogView.FLAG_EDIT_FAV_EDIT:
                mChannelDataManager.renameFavGroup(previousName, newName);
                newFavDisplay = newName;
                position = mChannelDataManager.getFavGroupPageIndex(newFavDisplay);
                break;
            default:
                break;
        }
        if (position < 0) {
            position = 0;
        }
        if (actionFlag >= CustomedDialogView.FLAG_EDIT_FAV_ADD && actionFlag <=CustomedDialogView.FLAG_EDIT_FAV_EDIT) {
            mFavListView.updateAllItem(this, mChannelDataManager.getEditFavListItem());
            mFavListView.requestFocus();
            mFavListView.setSelection(position);
        }
        if (newFavDisplay != null) {
            mLeftTitle.setText(newFavDisplay);
        } else {
            mLeftTitle.setText("");
            mAllListView.updateAllItem(SortFavActivity.this, new LinkedList<Item>());
        }
    }

    //add for service connect demo
    private IInterationService mClient = null;
    private InterationServiceConnection mInterationServiceConnection = new InterationServiceConnection();
    private boolean mIsBounded = false;
    private boolean mStartedBound = false;

    private final class InterationServiceConnection implements ServiceConnection {
        public void onServiceConnected(ComponentName name, IBinder service) {
            LOG(LOGD, null, "onServiceConnected name = " + name + ", service = " + service);
            mClient = IInterationService.Stub.asInterface(service);
            try {
                mClient.registerInterationCallback(mIInterationCallback);
            } catch (Exception e) {
                LOG(LOGD, null, "registerInterationCallback e = " + e.getMessage());
                e.printStackTrace();
            }
            mIsBounded = true;
        }
        public void onServiceDisconnected(ComponentName name) {
            LOG(LOGD, null, "onServiceDisconnected name = " + name);
            mIsBounded = false;
            try {
                mClient.unRegisterInterationCallback(mIInterationCallback);
            } catch (Exception e) {
                LOG(LOGD, null, "unRegisterInterationCallback e = " + e.getMessage());
                e.printStackTrace();
            }
            mClient = null;
        }
    }

    private IInterationCallback mIInterationCallback = new IInterationCallback.Stub() {
        @Override
        public void onReceiveInterationCallback(String json) {
            LOG(LOGD, null, "onReceiveInterationCallback = " + json);
        }
    };

    private void startInterationService() {
        LOG(LOGD, null, "startInterationService");
        Intent intent = new Intent("droidlogic.intent.action.InterationService");
        //intent.setClassName("com.droidlogic.droidlivetv", "com.droidlogic.droidlivetv.interationservice.InterationService");
        intent.setClassName("zhiwei.adilservice", "InterationService");
        startService(intent);
    }

    private void stopInterationService() {
        LOG(LOGD, null, "stopInterationService");
        Intent intent = new Intent("droidlogic.intent.action.InterationService");
        intent.setClassName("zhiwei.adilservice", "InterationService");
        stopService(intent);
    }

    private void bindInterationService() {
        if (!mStartedBound) {
            LOG(LOGD, null, "bindInterationService");
            mStartedBound = true;
            Intent intent = new Intent("droidlogic.intent.action.InterationService");
            intent.setClassName("zhiwei.adilservice", "InterationService");
            bindService(intent, mInterationServiceConnection, Service.BIND_AUTO_CREATE);
        } else {
            LOG(LOGD, null, "bindInterationService has started");
        }
    }

    private void unbindInterationService() {
        if (mStartedBound) {
            LOG(LOGD, null, "unbindInterationService");
            mStartedBound = false;
            unbindService(mInterationServiceConnection);
        } else {
            LOG(LOGD, null, "unbindInterationService not started");
        }
    }
}
