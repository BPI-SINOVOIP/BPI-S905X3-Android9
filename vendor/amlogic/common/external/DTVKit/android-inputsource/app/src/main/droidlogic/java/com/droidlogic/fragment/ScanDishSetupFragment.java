package com.droidlogic.fragment;

import java.util.LinkedList;
import java.util.Timer;
import java.util.TimerTask;

import com.droidlogic.fragment.ItemAdapter.ItemDetail;
import com.droidlogic.fragment.ItemListView.ListItemFocusedListener;
import com.droidlogic.fragment.ItemListView.ListItemSelectedListener;
import com.droidlogic.fragment.ItemListView.ListSwitchedListener;
import com.droidlogic.fragment.ItemListView.ListTypeSwitchedListener;
//import com.droidlogic.fragment.R.color;
import com.droidlogic.fragment.dialog.CustomDialog;
import com.droidlogic.fragment.dialog.DialogCallBack;
import com.droidlogic.fragment.dialog.DialogManager;

import android.app.AlertDialog;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.media.tv.TvInputInfo;
import android.widget.ProgressBar;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.widget.Toast;

import org.dtvkit.inputsource.R;
import org.dtvkit.inputsource.R.color;

public class ScanDishSetupFragment extends Fragment {

    private static final String TAG = "ScanDishSetupFragment";
    private ItemAdapter mItemAdapterItem = null;
    private ItemAdapter mItemAdapterOption = null;
    private ItemListView mListViewItem = null;
    private ItemListView mListViewOption = null;
    private LinkedList<ItemDetail> mItemDetailItem = new LinkedList<ItemDetail>();
    private LinkedList<ItemDetail> mItemDetailOption = new LinkedList<ItemDetail>();
    private String mCurrentListType = ParameterMananer.ITEM_SATALLITE;
    private String mCurrentListFocus = ItemListView.LIST_LEFT;
    private ParameterMananer mParameterMananer;
    private LinearLayout mSatelliteQuickkey;
    private LinearLayout mSatelliteQuickkey1;
    private LinearLayout mSatelliteQuickkey2;
    private TextView mItemTitleTextView;
    private TextView mOptionTitleItemTextView;
    private ProgressBar mStrengthProgressBar;
    private ProgressBar mQualityProgressBar;
    private TextView mStrengthTextView;
    private TextView mQualityTextView;
    private DialogManager mDialogManager = null;

    private TimerTask task = new TimerTask() {
        public void run() {
            ((ScanMainActivity)getActivity()).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (mParameterMananer == null) {
                        return;
                    }
                    int strength = mParameterMananer.getStrengthStatus();
                    int quality = mParameterMananer.getQualityStatus();
                    if (mStrengthProgressBar != null && mQualityProgressBar != null &&
                            mStrengthTextView != null && mQualityTextView != null) {
                        mStrengthProgressBar.setProgress(strength);
                        mQualityProgressBar.setProgress(quality);
                        mStrengthTextView.setText(strength + "%");
                        mQualityTextView.setText(quality + "%");
                        //Log.d(TAG, "run task get strength and quality");
                    }
                }
            });
        }
    };
    private Timer timer = new Timer();
    private boolean mScheduled = false;

    private HandlerThread mHandlerThread;
    private Handler  mThreadHandler;

    private final int MSG_START_TUNE_ACTION = 0;
    private final int MSG_STOP_TUNE_ACTION = 1;
    private final int MSG_STOP_RELEAS_ACTION = 2;
    private final int VALUE_START_TUNE_DELAY = 200;
    private boolean mIsStarted = false;
    private boolean mIsReleasing = false;

    private void initHandlerThread() {
        mHandlerThread = new HandlerThread("check-message-coming");
        mHandlerThread.start();
        mThreadHandler = new Handler(mHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_START_TUNE_ACTION:
                        if (mParameterMananer != null) {
                            mIsStarted = true;
                            mParameterMananer.startTuneAction();
                        }
                        break;
                    case MSG_STOP_TUNE_ACTION:
                        if (mParameterMananer != null) {
                            mIsStarted = false;
                            mParameterMananer.stopTuneAction();
                        }
                        break;
                    case MSG_STOP_RELEAS_ACTION:
                        if (!mThreadHandler.hasMessages(MSG_STOP_RELEAS_ACTION)) {
                            releaseHandler();
                        } else {
                            releaseMessage();
                        }
                        break;
                    default:
                        break;
                }
            }
        };
    }

    /*public static ScanDishSetupFragment newInstance() {
        return new ScanDishSetupFragment();
    }*/
    public ScanDishSetupFragment() {

    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initHandlerThread();
        Log.d(TAG, "onCreate");
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        Log.d(TAG, "onCreateView");
        mParameterMananer = ((ScanMainActivity)getActivity()).getParameterMananer();
        mDialogManager = ((ScanMainActivity)getActivity()).getDialogManager();
        LinkedList<ItemDetail> satellitelist = mParameterMananer.getSatelliteList();
        LinkedList<ItemDetail> transponderlist = mParameterMananer.getTransponderList();
        String currentlist = mParameterMananer.getCurrentListType();
        View rootView = inflater.inflate(R.layout.fragment_dish_setup, container, false);
        mSatelliteQuickkey1 = (LinearLayout) rootView.findViewById(R.id.function_key1);
        mSatelliteQuickkey2 = (LinearLayout) rootView.findViewById(R.id.function_key2);
        creatFour1();
        creatFour2();

        mStrengthProgressBar = (ProgressBar)rootView.findViewById(R.id.strength_progressbar);
        mQualityProgressBar = (ProgressBar)rootView.findViewById(R.id.quality_progressbar);
        mStrengthTextView = (TextView)rootView.findViewById(R.id.strength_percent);
        mQualityTextView = (TextView)rootView.findViewById(R.id.quality_percent);

        mListViewItem = (ItemListView) rootView.findViewById(R.id.listview_item);
        mListViewOption = (ItemListView) rootView.findViewById(R.id.listview_option);
        mItemDetailItem.addAll(mParameterMananer.getItemList(currentlist));
        mItemAdapterItem = new ItemAdapter(mItemDetailItem, getActivity());
        mListViewItem.setAdapter(mItemAdapterItem);
        mListViewItem.setCurrentListSide(ItemListView.LIST_LEFT);

        mItemTitleTextView = (TextView) rootView.findViewById(R.id.listview_item_title);
        mItemTitleTextView.setText(ItemListView.ITEM_SATALLITE.equals(currentlist) ? R.string.list_type_satellite : R.string.list_type_transponder);

        mListViewItem.setListItemSelectedListener(mListItemSelectedListener);
        mListViewItem.setListItemFocusedListener(mListItemFocusedListener);
        mListViewItem.setListSwitchedListener(mListSwitchedListener);
        mListViewItem.setListTypeSwitchedListener(mListTypeSwitchedListener);
        mListViewItem.setDataCallBack(mSingleSelectDialogCallBack);
        mListViewItem.setDtvkitGlueClient(((ScanMainActivity)getActivity()).getDtvkitGlueClient());

        mItemDetailOption.addAll(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
        mItemAdapterOption = new ItemAdapter(mItemDetailOption, getActivity());
        mListViewOption.setAdapter(mItemAdapterOption);
        mListViewOption.setCurrentListSide(ItemListView.LIST_RIGHT);
        mListViewItem.setSelection(ItemListView.ITEM_SATALLITE.equals(mParameterMananer.getCurrentListType()) ? mParameterMananer.getCurrentSatelliteIndex() : mParameterMananer.getCurrentTransponderIndex());

        //mOptionTitleItemTextView = (TextView) rootView.findViewById(R.id.listview_option_title);
        //mOptionTitleItemTextView.setText(mParameterMananer.getParameterListTitle(mParameterMananer.getCurrentListType(), ITEM_SATALLITE.equals(mParameterMananer.getCurrentListType()) ? mParameterMananer.getCurrentSatelliteIndex() : mParameterMananer.getCurrentTransponderIndex())/*"Ku_NewSat2"*/);

        //mListViewOption.setSelectionAfterHeaderView();
        mListViewOption.setListItemSelectedListener(mListItemSelectedListener);
        mListViewOption.setListItemFocusedListener(mListItemFocusedListener);
        mListViewOption.setListSwitchedListener(mListSwitchedListener);
        mListViewOption.setListTypeSwitchedListener(mListTypeSwitchedListener);
        mListViewOption.setListType(ItemListView.ITEM_OPTION);
        mListViewOption.setDataCallBack(mSingleSelectDialogCallBack);
        //mListViewOption.cleanChoosed();
        /*if (!((TextUtils.equals(currentlist, ItemListView.ITEM_SATALLITE) && satellitelist != null && satellitelist.size() > 0) ||
                (TextUtils.equals(currentlist, ItemListView.ITEM_TRANSPONDER) && transponderlist != null && transponderlist.size() > 0))) {
            mListViewOption.requestFocus();
            View selectedView = mListViewOption.getSelectedView();
            if (selectedView != null) {
                mListViewOption.setChoosed(selectedView);
            }
        } else {
            mListViewOption.cleanChoosed();
        }*/

        if ((TextUtils.equals(currentlist, ItemListView.ITEM_SATALLITE) && satellitelist != null && satellitelist.size() > 0) ||
                (TextUtils.equals(currentlist, ItemListView.ITEM_TRANSPONDER) && transponderlist != null && transponderlist.size() > 0)) {
            mListViewItem.requestFocus();
            mListViewOption.cleanChoosed();
            mParameterMananer.setCurrentListDirection("left");
            View selectedView = mListViewItem.getSelectedView();
            if (selectedView != null) {
                mListViewItem.setChoosed(selectedView);
            }
            mListViewItem.setListType(mParameterMananer.getCurrentListType());
        } else {
            mListViewItem.cleanChoosed();
            mCurrentListFocus = "right";
            mParameterMananer.setCurrentListDirection("right");
            View selectedView = mListViewOption.getSelectedView();
            if (selectedView != null) {
                mListViewOption.setChoosed(selectedView);
            }
        }

        mListViewOption.setOnItemSelectedListener(mListViewOption);
        mListViewItem.setOnItemSelectedListener(mListViewItem);
        initStrengthQualityUpdate();

        return rootView;
    }

    private void releaseHandler() {
        Log.d(TAG, "releaseHandler");
        mIsReleasing = true;
        getActivity().finish();
        if (mThreadHandler != null) {
            mThreadHandler.removeCallbacksAndMessages(null);
            mThreadHandler = null;
        }

        if (mHandlerThread != null) {
            mHandlerThread.quit();
            mHandlerThread = null;
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        if (mScheduled) {
            abortStrengthQualityUpdate();
        }
        if (mIsStarted) {
            mIsStarted = false;
            stopTune();
        }
        if (!mIsReleasing) {
            mIsReleasing = true;
            releaseMessage();
        }
        Log.d(TAG, "onStop");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        //releaseHandler();
        //mParameterMananer.stopTuneAction();
        Log.d(TAG, "onDestroy");
    }

    private void changeSatelliteQuickkeyLayout() {
        mSatelliteQuickkey.removeAllViews();
        mSatelliteQuickkey.addView(mSatelliteQuickkey1);
        mSatelliteQuickkey.addView(mSatelliteQuickkey2);
    }

    private void creatFour1() {
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = (View) inflater.inflate(R.layout.four_display1, null);
        mSatelliteQuickkey1.removeAllViews();
        mSatelliteQuickkey1.addView(view);
    }

    private void creatFour2() {
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = (View) inflater.inflate(R.layout.four_display2, null);
        mSatelliteQuickkey2.removeAllViews();
        mSatelliteQuickkey2.addView(view);
    }

    private void creatConfirmandExit1() {
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = (View) inflater.inflate(R.layout.confirm_exit_display, null);
        mSatelliteQuickkey1.removeAllViews();
        mSatelliteQuickkey1.addView(view);
    }

    private void creatSatelliteandScan2() {
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = (View) inflater.inflate(R.layout.satellite_scan_display, null);
        mSatelliteQuickkey2.removeAllViews();
        mSatelliteQuickkey2.addView(view);
    }

    private void creatSetlimitandSetlocation1() {
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = (View) inflater.inflate(R.layout.limit_location_display, null);
        mSatelliteQuickkey1.removeAllViews();
        mSatelliteQuickkey1.addView(view);
    }

    private void creatEditandExit2() {
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = (View) inflater.inflate(R.layout.edit_exit_wheel_display, null);
        mSatelliteQuickkey2.removeAllViews();
        mSatelliteQuickkey2.addView(view);
    }

    private void initStrengthQualityUpdate() {
        startTune();
        if (mScheduled) {
            abortStrengthQualityUpdate();
        }
        timer.schedule(task, 1000, 1000);
        mScheduled = true;
        Log.d(TAG, "initStrengthQualityUpdate");
    }

    private void abortStrengthQualityUpdate() {
        if (mScheduled) {
            mScheduled = false;
            task.cancel();
            timer.cancel();
            Log.d(TAG, "abortStrengthQualityUpdate");
        }
    }

    /*AdapterView.OnItemClickListener mOnItemClickListener = new AdapterView.OnItemClickListener() {

        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            // TODO Auto-generated method stub
            Log.d(TAG, "onItemClick view = " + view.getLabelFor() + ", position = " + position + ", id = " + id);
        }
    };

    AdapterView.OnItemSelectedListener mOnItemSelectedListener = new AdapterView.OnItemSelectedListener() {

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            // TODO Auto-generated method stub
            Log.d(TAG, "onItemSelected view = " + view.getLabelFor() + ", position = " + position + ", id = " + id + ", " + mListViewItem.getSelectedItemPosition());
            //view.setBackgroundColor(color.common_focus);

        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            // TODO Auto-generated method stub
            Log.d(TAG, "onNothingSelected onNothingSelected");
        }
    };

    OnFocusChangeListener mOnFocusChangeListener = new OnFocusChangeListener() {

        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            // TODO Auto-generated method stub
            Log.d(TAG, "onFocusChange view = " + v.getLabelFor() + ", hasFocus = " + hasFocus);
        }
    };*/

    private CustomDialog mCurrentCustomDialog = null;
    private CustomDialog mCurrentSubCustomDialog = null;
    private DialogCallBack mSingleSelectDialogCallBack = new DialogCallBack() {
        @Override
        public void onStatusChange(View view, String parameterKey, Bundle data) {
            Log.d(TAG, "onStatusChange parameterKey = " + parameterKey + ", data = " + data);
            switch (parameterKey) {
                case ParameterMananer.KEY_LNB_TYPE:
                    if (data != null && "selected".equals(data.getString("action"))) {
                        mParameterMananer.saveIntParameters(parameterKey, data.getInt("position"));
                        if (mCurrentCustomDialog != null && TextUtils.equals(parameterKey, mCurrentCustomDialog.getDialogKey())) {
                            mCurrentCustomDialog.updateListView(mCurrentCustomDialog.getDialogTitle(), mCurrentCustomDialog.getDialogKey(), data.getInt("position"));
                            //mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                        }
                        if (data.getInt("position") == 2) {
                            mCurrentSubCustomDialog = mDialogManager.buildLnbCustomedItemDialog(mSingleSelectDialogCallBack);
                            if (mCurrentSubCustomDialog != null) {
                                mCurrentSubCustomDialog.showDialog();
                            }
                        } else {
                            startTune();
                        }
                        mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                    }
                    break;
                case ParameterMananer.KEY_LNB_CUSTOM:
                    if (data != null && "onClick".equals(data.getString("action"))) {
                        if ("ok".equals(data.getString("button"))) {
                            Log.d(TAG, "ok in clicked");
                            if (!TextUtils.isEmpty(data.getString("value1")) && !TextUtils.isEmpty(data.getString("value2"))) {
                                mParameterMananer.saveStringParameters(parameterKey, data.getString("value1") + "," + data.getString("value2"));
                            } else if (!TextUtils.isEmpty(data.getString("value1")) && TextUtils.isEmpty(data.getString("value2"))) {
                                mParameterMananer.saveStringParameters(parameterKey, data.getString("value1"));
                            } else {
                                mParameterMananer.saveStringParameters(parameterKey, "");
                            }
                            startTune();
                        } else if ("cancel".equals(data.getString("button"))) {
                            Log.d(TAG, "cancel in clicked");
                        }
                        mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                    }
                    break;
                case ParameterMananer.KEY_UNICABLE:
                    if (data != null) {
                        mParameterMananer.saveIntParameters(ParameterMananer.KEY_UNICABLE, data.getInt("position"));
                    }
                    if (data != null && data.getInt("position") != 2  && ("left".equals(data.getString("action")) || "right".equals(data.getString("action")))) {
                        int min = 0;
                        int max = 1;
                        int unicable_position = data.getInt("position");
                        if (unicable_position == 1) {
                            min = 0;
                            max = 7;
                        }
                        if ("left".equals(data.getString("action"))) {
                            Log.d(TAG, "unicable switch left in clicked");
                            int unicable_value = mParameterMananer.getIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[unicable_position]);
                            if (unicable_value > min) {
                                unicable_value--;
                            }
                            mParameterMananer.saveIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[unicable_position], unicable_value);
                            //update UB frequency when set user band
                            if (unicable_position == 1) {
                                mParameterMananer.saveIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[2], CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_USER_BAND_FREQUENCY_LIST[unicable_value]);
                            }
                        } else if ("right".equals(data.getString("action"))) {
                            Log.d(TAG, "unicable switch right in clicked");
                            int unicable_value1 = mParameterMananer.getIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[unicable_position]);
                            if (unicable_value1 < max) {
                                unicable_value1++;
                            }
                            mParameterMananer.saveIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[unicable_position], unicable_value1);
                            //update UB frequency when set user band
                            if (unicable_position == 1) {
                                mParameterMananer.saveIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[2], CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_USER_BAND_FREQUENCY_LIST[unicable_value1]);
                            }
                        }
                        if (mCurrentCustomDialog != null) {
                            mCurrentCustomDialog.updateListView(mCurrentCustomDialog.getDialogTitle(), mCurrentCustomDialog.getDialogKey(), mParameterMananer.getIntParameters(ParameterMananer.KEY_UNICABLE));
                        }
                        mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                        startTune();
                    } else if ("selected".equals(data.getString("action")) && data != null && data.getInt("position") == 2) {
                        Log.d(TAG, "unicable UB frequency display only");
                        /*mCurrentSubCustomDialog = mDialogManager.buildUnicableCustomedItemDialog(mSingleSelectDialogCallBack);
                        if (mCurrentSubCustomDialog != null) {
                            mCurrentSubCustomDialog.showDialog();
                        }
                        mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));*/
                    }

                    break;
                case ParameterMananer.KEY_UB_FREQUENCY:
                    if (data != null && "onClick".equals(data.getString("action"))) {
                        if ("ok".equals(data.getString("button"))) {
                            Log.d(TAG, "KEY_UB_FREQUENCY ok in clicked");
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_UB_FREQUENCY, Integer.valueOf(data.getString("value1")));
                            if (mCurrentCustomDialog != null) {
                                mCurrentCustomDialog.updateListView(mCurrentCustomDialog.getDialogTitle(), mCurrentCustomDialog.getDialogKey(), mParameterMananer.getIntParameters(ParameterMananer.KEY_UNICABLE));
                            }
                            startTune();
                        } else if ("cancel".equals(data.getString("button"))) {
                            Log.d(TAG, "KEY_UB_FREQUENCY cancel in clicked");
                        }
                    }
                    break;
                case ParameterMananer.KEY_LNB_POWER:
                case ParameterMananer.KEY_22_KHZ:
                case ParameterMananer.KEY_TONE_BURST:
                case ParameterMananer.KEY_DISEQC1_0:
                case ParameterMananer.KEY_DISEQC1_1:
                    if (data != null && "selected".equals(data.getString("action"))) {
                        mParameterMananer.saveIntParameters(parameterKey, data.getInt("position"));
                        if (mCurrentCustomDialog != null && TextUtils.equals(parameterKey, mCurrentCustomDialog.getDialogKey())) {
                            mCurrentCustomDialog.updateListView(mCurrentCustomDialog.getDialogTitle(), mCurrentCustomDialog.getDialogKey(), data.getInt("position"));
                        }
                        mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                        startTune();
                    }
                    break;
                case ParameterMananer.KEY_MOTOR:
                    if (data != null && "selected".equals(data.getString("action"))) {
                        mParameterMananer.saveIntParameters(parameterKey, data.getInt("position"));
                        if (data.getInt("position") == 0 && mCurrentCustomDialog != null && TextUtils.equals(parameterKey, mCurrentCustomDialog.getDialogKey())) {
                            mCurrentCustomDialog.updateListView(mCurrentCustomDialog.getDialogTitle(), mCurrentCustomDialog.getDialogKey(), data.getInt("position"));
                            //mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatellite()));
                        } else if (data.getInt("position") == 1 && mCurrentCustomDialog != null && TextUtils.equals(parameterKey, mCurrentCustomDialog.getDialogKey())){
                            mCurrentCustomDialog.updateListView(mCurrentCustomDialog.getDialogTitle(), mCurrentCustomDialog.getDialogKey(), data.getInt("position"));
                            mCurrentSubCustomDialog = mDialogManager.buildDiseqc1_2_ItemDialog(mSingleSelectDialogCallBack);
                            if (mCurrentSubCustomDialog != null) {
                                mCurrentSubCustomDialog.showDialog();
                            }
                        }
                        mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                        startTune();
                    }
                    break;
                case ParameterMananer.KEY_DISEQC1_2:
                    if (data != null) {
                        mParameterMananer.saveIntParameters(parameterKey, data.getInt("position"));
                    }
                    if (data != null && "selected".equals(data.getString("action"))) {
                        int diseqc1_2_position = data.getInt("position");
                        mParameterMananer.saveIntParameters(parameterKey, diseqc1_2_position);
                        boolean needbreak = false;
                        switch (diseqc1_2_position) {
                            case 3://dish limts east
                                mParameterMananer.setDishELimits();
                                needbreak = true;
                                break;
                            case 4://dish limts west
                                mParameterMananer.setDishWLimits();
                                needbreak = true;
                                break;
                            case 7://move
                                mParameterMananer.dishMove(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_MOVE_DIRECTION), mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_MOVE_STEP));
                                needbreak = true;
                                break;
                            case 9://save to position
                                mParameterMananer.storeDishPosition(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_CURRENT_POSITION));
                                needbreak = true;
                                break;
                            case 10://move to position
                                mParameterMananer.moveDishToPosition(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_CURRENT_POSITION));
                                needbreak = true;
                                break;
                            default:
                                needbreak = true;
                                break;
                        }
                        if (!needbreak && mCurrentSubCustomDialog != null && mCurrentSubCustomDialog.isShowing()) {
                            mCurrentSubCustomDialog.updateDiseqc1_2_Dialog();
                        } else {
                            Log.d(TAG, "mCurrentSubCustomDialog null or need break or not displayed");
                        }
                    } else if (data != null && ("left".equals(data.getString("action")) || "right".equals(data.getString("action")))) {
                        int position = data.getInt("position");
                        boolean needbreak = false;
                        switch (position) {
                            case 2://dish limts status
                                mParameterMananer.saveIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_LIMITS_STATUS, "left".equals(data.getString("action")) ? 0 : 1);
                                mParameterMananer.enableDishLimits(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_LIMITS_STATUS) == 1);
                                break;
                            /*case 3://dish limts east
                                int value = mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_EAST_LIMITS);
                                if ("left".equals(data.getString("action"))) {
                                    if (value != 0) {
                                        value = value - 1;
                                    }
                                } else {
                                    if (value != 180) {
                                        value = value + 1;
                                    }
                                }
                                mParameterMananer.saveIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_EAST_LIMITS, value);
                                mParameterMananer.setDishLimits(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_EAST_LIMITS), mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_WEST_LIMITS));
                                break;
                            case 4://dish limts west
                                int westvalue = mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_WEST_LIMITS);
                                if ("left".equals(data.getString("action"))) {
                                    if (westvalue != 0) {
                                        westvalue = westvalue - 1;
                                    }
                                } else {
                                    if (westvalue != 180) {
                                        westvalue = westvalue + 1;
                                    }
                                }
                                mParameterMananer.saveIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_WEST_LIMITS, westvalue);
                                mParameterMananer.setDishLimits(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_EAST_LIMITS), mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_WEST_LIMITS));
                                break;*/
                            case 5://dish move direction
                                int directionvalue = mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_MOVE_DIRECTION);
                                if ("left".equals(data.getString("action"))) {
                                    if (directionvalue != 0) {
                                        directionvalue = directionvalue - 1;
                                    }
                                } else {
                                    if (directionvalue != 2) {
                                        directionvalue = directionvalue + 1;
                                    }
                                }
                                mParameterMananer.saveIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_MOVE_DIRECTION, directionvalue);
                                break;
                            case 6://dish move step
                                int stepvalue = mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_MOVE_STEP);
                                if ("left".equals(data.getString("action"))) {
                                    if (stepvalue != 0) {
                                        stepvalue = stepvalue - 1;
                                    }
                                } else {
                                    if (stepvalue != 127) {
                                        stepvalue = stepvalue + 1;
                                    }
                                }
                                mParameterMananer.saveIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_MOVE_STEP, stepvalue);
                                break;
                            case 8://dish position
                                int positionvalue = mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_CURRENT_POSITION);
                                if ("left".equals(data.getString("action"))) {
                                    if (positionvalue != 0) {
                                        positionvalue = positionvalue - 1;
                                    }
                                } else {
                                    if (positionvalue != 255) {
                                        positionvalue = positionvalue + 1;
                                    }
                                }
                                mParameterMananer.saveIntParameters(ParameterMananer.KEY_DISEQC1_2_DISH_CURRENT_POSITION, positionvalue);
                                break;
                            default:
                                needbreak = true;
                                break;
                        }
                        if (!needbreak && mCurrentSubCustomDialog != null && mCurrentSubCustomDialog.isShowing()) {
                            mCurrentSubCustomDialog.updateDiseqc1_2_Dialog();
                        } else {
                            Log.d(TAG, "mCurrentSubCustomDialog null or need break or not displayed");
                        }
                    }
                    break;
                case ParameterMananer.KEY_FUNCTION:
                    if (data != null) {
                        String action = data.getString("action");
                        String listtype = data.getString("listtype");
                        switch (action) {
                            case "add":
                                if (ItemListView.ITEM_SATALLITE.equals(data.getString("listtype"))) {
                                    mCurrentCustomDialog = mDialogManager.buildAddSatelliteDialogDialog(null, mSingleSelectDialogCallBack);
                                } else if (ItemListView.ITEM_TRANSPONDER.equals(data.getString("listtype"))) {
                                    mCurrentCustomDialog = mDialogManager.buildAddTransponderDialogDialog(null, mSingleSelectDialogCallBack);
                                } else {
                                    Log.d(TAG, "not sure");
                                    mCurrentCustomDialog = null;
                                }
                                if (mCurrentCustomDialog != null) {
                                    mCurrentCustomDialog.showDialog();
                                }
                                break;
                            case "edit":
                                if (ItemListView.ITEM_SATALLITE.equals(data.getString("listtype"))) {
                                    mCurrentCustomDialog = mDialogManager.buildAddSatelliteDialogDialog(data.getString("parameter"), mSingleSelectDialogCallBack);
                                } else if (ItemListView.ITEM_TRANSPONDER.equals(data.getString("listtype"))) {
                                    mCurrentCustomDialog = mDialogManager.buildAddTransponderDialogDialog(data.getString("parameter"), mSingleSelectDialogCallBack);
                                } else {
                                    Log.d(TAG, "not sure");
                                    mCurrentCustomDialog = null;
                                }
                                if (mCurrentCustomDialog != null) {
                                    mCurrentCustomDialog.showDialog();
                                }
                                break;
                            case "delete":
                                if (ItemListView.ITEM_SATALLITE.equals(data.getString("listtype"))) {
                                    mCurrentCustomDialog = mDialogManager.buildRemoveSatelliteDialogDialog(data.getString("parameter"), mSingleSelectDialogCallBack);
                                } else if (ItemListView.ITEM_TRANSPONDER.equals(data.getString("listtype"))) {
                                    mCurrentCustomDialog = mDialogManager.buildRemoveTransponderDialogDialog(data.getString("parameter"), mSingleSelectDialogCallBack);
                                } else {
                                    Log.d(TAG, "not sure");
                                    mCurrentCustomDialog = null;
                                }
                                if (mCurrentCustomDialog != null) {
                                    mCurrentCustomDialog.showDialog();
                                }
                                break;
                            case "scan":
                                /*Intent intent = new Intent();
                                intent.setClassName("org.dtvkit.inputsource", "org.dtvkit.inputsource.DtvkitDvbsSetup");
                                String inputId = getActivity().getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
                                if (inputId != null) {
                                    intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
                                }
                                getActivity().startActivity(intent);*/
                                //getActivity().startActivityForResult(intent, ScanMainActivity.REQUEST_CODE_START_SETUP_ACTIVITY);
                                //getActivity().finish();
                                stopAndRelease();
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case ParameterMananer.KEY_ADD_SATELLITE:
                    if (data != null && "ok".equals(data.getString("button"))) {
                        String name_add = data.getString("value1");
                        boolean iseast_add = data.getBoolean("value2", true);
                        int position_add = Integer.valueOf(data.getString("value3"));
                        mParameterMananer.addSatellite(name_add, iseast_add, position_add);
                        mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
                        switchtoLeftList();
                    }
                    break;
                case ParameterMananer.KEY_EDIT_SATELLITE:
                    if (data != null && "ok".equals(data.getString("button"))) {
                        String name_edit = data.getString("value1");
                        boolean iseast_edit = data.getBoolean("value2", true);
                        int position_edit = Integer.valueOf(data.getString("value3"));
                        mParameterMananer.addSatellite(name_edit, iseast_edit, position_edit);
                        mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
                    }
                    break;
                case ParameterMananer.KEY_REMOVE_SATELLITE:
                    if (data != null && "ok".equals(data.getString("button"))) {
                        String name_remove = data.getString("value1");
                        mParameterMananer.removeSatellite(name_remove);
                        //Log.d(TAG, "KEY_REMOVE_SATELLITE = " + mParameterMananer.getCurrentSatellite());
                        if (TextUtils.equals(name_remove, mParameterMananer.getCurrentSatellite())) {
                            mParameterMananer.setCurrentSatellite("null");
                            mParameterMananer.setCurrentTransponder("null");
                            mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                        }
                        mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
                    }
                    break;
                case ParameterMananer.KEY_ADD_TRANSPONDER:
                    if (data != null && "ok".equals(data.getString("button"))) {
                        String name_add_t = data.getString("value1");
                        int frequency_add_t = Integer.valueOf(data.getString("value2"));
                        String polarity_add_t = data.getString("value3");
                        int symbol_add_t = Integer.valueOf(data.getString("value4"));
                        mParameterMananer.addTransponder(name_add_t, frequency_add_t, polarity_add_t, symbol_add_t);
                        mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
                        switchtoLeftList();
                    }
                    break;
                case ParameterMananer.KEY_EDIT_TRANSPONDER:
                    if (data != null && "ok".equals(data.getString("button"))) {
                        String name_edit_t = data.getString("value1");
                        int frequency_edit_t = Integer.valueOf(data.getString("value2"));
                        String polarity_edit_t = data.getString("value3");
                        int symbol_edit_t = Integer.valueOf(data.getString("value4"));
                        mParameterMananer.addTransponder(name_edit_t, frequency_edit_t, polarity_edit_t, symbol_edit_t);
                        mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
                    }
                    break;
                case ParameterMananer.KEY_REMOVE_TRANSPONDER:
                    if (data != null && "ok".equals(data.getString("button"))) {
                        String name_remove_t = data.getString("value1");
                        int name_frequency_t = Integer.valueOf(data.getString("value2"));
                        String name_polarity_t = data.getString("value3");
                        int name_symbol_t = Integer.valueOf(data.getString("value4"));
                        mParameterMananer.removeTransponder(name_remove_t, name_frequency_t, name_polarity_t, name_symbol_t);
                        //Log.d(TAG, "KEY_REMOVE_TRANSPONDER = " + mParameterMananer.getCurrentSatellite() + ", trans = " + mParameterMananer.getCurrentTransponder());
                        if (TextUtils.equals(name_remove_t, mParameterMananer.getCurrentSatellite())) {
                            String removeTrans = name_frequency_t + "/" + name_polarity_t + "/" + name_symbol_t;
                            if (TextUtils.equals(removeTrans, mParameterMananer.getCurrentTransponder())) {
                                mParameterMananer.setCurrentTransponder("null");
                                mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
                            }
                        }
                        mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private void startTune() {
        if (mThreadHandler != null) {
            if (mThreadHandler.hasMessages(MSG_START_TUNE_ACTION)) {
                mThreadHandler.removeMessages(MSG_START_TUNE_ACTION);
            }
            Log.d(TAG, "sendEmptyMessage startTune");
            mThreadHandler.sendEmptyMessageDelayed(MSG_START_TUNE_ACTION, VALUE_START_TUNE_DELAY);
        }
    }

    private void stopTune() {
        if (mThreadHandler != null) {
            if (mThreadHandler.hasMessages(MSG_START_TUNE_ACTION)) {
                mThreadHandler.removeMessages(MSG_START_TUNE_ACTION);
            }
            Log.d(TAG, "sendEmptyMessage stopTune");
            mThreadHandler.sendEmptyMessage(MSG_STOP_TUNE_ACTION);
        }
    }

    private void stopAndRelease() {
        Log.d(TAG, "stopAndRelease");
        abortStrengthQualityUpdate();
        stopTune();
        releaseMessage();
    }

    private void releaseMessage() {
        Log.d(TAG, "releaseMessage");
        if (mThreadHandler != null) {
            if (mThreadHandler.hasMessages(MSG_STOP_RELEAS_ACTION)) {
                mThreadHandler.removeMessages(MSG_STOP_RELEAS_ACTION);
            }
            mThreadHandler.sendEmptyMessage(MSG_STOP_RELEAS_ACTION);
        }
    }

    private void switchtoLeftList() {
        if (ItemListView.LIST_RIGHT.equals(mCurrentListFocus)) {
            mCurrentListFocus = ItemListView.LIST_LEFT;
        }
        if (ItemListView.LIST_LEFT.equals(mCurrentListFocus)) {
            mListViewOption.cleanChoosed();
            mListViewItem.cleanChoosed();
            mListViewItem.requestFocus();
            if (mListViewItem.getSelectedView() != null) {
                mListViewItem.setChoosed(mListViewItem.getSelectedView());
            }
            creatFour1();
            creatFour2();
            mParameterMananer.setCurrentListDirection(ItemListView.LIST_LEFT);
        }
    }

    ListItemSelectedListener mListItemSelectedListener = new ListItemSelectedListener() {

        @Override
        public void onListItemSelected(int position, String type) {
            Log.d(TAG, "onListItemSelected position = " + position + ", type = " + type);
            if (ItemListView.LIST_LEFT.equals(mCurrentListFocus)) {
                String listtype = mParameterMananer.getCurrentListType();
                if (ItemListView.ITEM_SATALLITE.equals(listtype)) {
                    LinkedList<ItemDetail> items = mParameterMananer.getSatelliteList();
                    if (items != null && position < items.size()) {
                        mParameterMananer.setCurrentSatellite(items != null ? items.get(position).getFirstText() : "null");
                        mParameterMananer.setCurrentTransponder("null");
                    } else {
                        Log.e(TAG, "onListItemSelected setCurrentSatellite erro position index");
                    }
                } else if (ParameterMananer.ITEM_TRANSPONDER.equals(listtype)) {
                    LinkedList<ItemDetail> items = mParameterMananer.getTransponderList();
                    if (items != null && position < items.size()) {
                        String transponder = items != null ? (items.get(position).getFirstText()) : "null";
                        mParameterMananer.setCurrentTransponder(transponder);
                        if (!TextUtils.isEmpty(transponder) && !"null".equals(transponder)) {
                            startTune();
                        }
                    } else {
                        Log.e(TAG, "onListItemSelected setCurrentTransponder erro position index");
                    }
                }
                mItemAdapterItem.reFill(mParameterMananer.getItemList(listtype));
                mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), mParameterMananer.getCurrentSatelliteIndex()));
            } else if (ItemListView.LIST_RIGHT.equals(mCurrentListFocus)) {
                if (position == 0 || position == 1) {
                    Log.d(TAG, "satellite or transponder no sub menu");
                    return;
                }
                mCurrentCustomDialog = mDialogManager.buildItemDialogById(position, mSingleSelectDialogCallBack);
                if (mCurrentCustomDialog != null) {
                    mCurrentCustomDialog.showDialog();
                }
                /*int optionitemIndex = position;
                AlertDialog singleSelectDialog = null;
                switch (optionitemIndex) {
                    case 0:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_SATALLITE, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 1:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_TRANSPONDER, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 2:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_LNB_TYPE, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 3:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_UNICABLE, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 4:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_LNB_POWER, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 5:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_22_KHZ, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 6:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_TONE_BURST, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 7:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_DISEQC1_0, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 8:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_DISEQC1_1, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    case 9:
                        singleSelectDialog = mDialogManager.buildSelectSingleItemDialog(ParameterMananer.KEY_MOTOR, mSingleSelectDialogCallBack);
                        singleSelectDialog.show();
                        break;
                    default:
                        break;
                }*/
            }
        }
    };

    ListItemFocusedListener mListItemFocusedListener = new ListItemFocusedListener() {

        @Override
        public void onListItemFocused(View parent, int position, String type) {
            Log.d(TAG, "onListItemFocused position = " + position + ", type = " + type);
            /*if (ItemListView.LIST_LEFT.equals(mCurrentListFocus) && ItemListView.isRightList(type)) {
                mListViewOption.cleanChoosed();
            }*/
            if (ItemListView.LIST_LEFT.equals(mCurrentListFocus)) {
                /*mItemDetailOption.clear();
                mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(type, position));
                if (mItemAdapterOption.getCount() > 0) {
                    mListViewOption.setSelection(0);
                }*/
                mListViewItem.cleanChoosed();
                mListViewOption.cleanChoosed();
                View selectedView = mListViewItem.getSelectedView();
                if (selectedView != null) {
                    selectedView.requestFocus();
                    mListViewItem.setChoosed(selectedView);
                }
            } else if (ItemListView.LIST_RIGHT.equals(mCurrentListFocus)) {
                mListViewItem.cleanChoosed();
            }
        }
    };

    ListSwitchedListener mListSwitchedListener = new ListSwitchedListener() {

        @Override
        public void onListSwitched(String direction) {
            Log.d(TAG, "onListSwitched direction = " + direction);
            if (direction != null) {
                mCurrentListFocus = direction;
            }
            if (ItemListView.LIST_LEFT.equals(mCurrentListFocus)) {
                mListViewOption.cleanChoosed();
                mListViewItem.requestFocus();
                if (mListViewItem.getSelectedView() != null) {
                    mListViewItem.setChoosed(mListViewItem.getSelectedView());
                }
                creatFour1();
                creatFour2();
                mParameterMananer.setCurrentListDirection("left");
            } else if (ItemListView.LIST_RIGHT.equals(mCurrentListFocus)) {
                mListViewItem.cleanChoosed();
                mListViewOption.requestFocus();
                mListViewOption.setChoosed(mListViewOption.getSelectedView());
                creatConfirmandExit1();
                creatSatelliteandScan2();
                mParameterMananer.setCurrentListDirection("right");
            }
            if (ParameterMananer.ITEM_SATALLITE.equals(mCurrentListType)) {
                mListViewItem.setSelection(mParameterMananer.getCurrentSatelliteIndex());
            } else if (ParameterMananer.ITEM_TRANSPONDER.equals(mCurrentListType)) {
                mListViewItem.setSelection(mParameterMananer.getCurrentTransponderIndex());
            }
        }
    };

    ListTypeSwitchedListener mListTypeSwitchedListener = new ListTypeSwitchedListener() {
        @Override
        public void onListTypeSwitched(String listtype) {
            Log.d(TAG, "onListTypeSwitched listtype = " + listtype);
            mCurrentListType = listtype;
            mParameterMananer.setCurrentListType(mCurrentListType);
            mItemAdapterItem.reFill(mParameterMananer.getItemList(mParameterMananer.getCurrentListType()));
            mListViewItem.setAdapter(mItemAdapterItem);
            mListViewItem.cleanChoosed();
            if (ItemListView.ITEM_SATALLITE.equals(mCurrentListType)) {
                mListViewItem.setSelection(mParameterMananer.getCurrentSatelliteIndex());
                View selectedView = mListViewItem.getSelectedView();
                if (selectedView != null) {
                    selectedView.requestFocus();
                    mListViewItem.setChoosed(selectedView);
                }
            } else if ((ItemListView.ITEM_TRANSPONDER.equals(mCurrentListType))) {
                mListViewItem.setSelection(mParameterMananer.getCurrentTransponderIndex());
                View selectedView = mListViewItem.getSelectedView();
                if (selectedView != null) {
                    selectedView.requestFocus();
                    mListViewItem.setChoosed(selectedView);
                }
            }
            mItemTitleTextView.setText(ItemListView.ITEM_SATALLITE.equals(mParameterMananer.getCurrentListType()) ? R.string.list_type_satellite : R.string.list_type_transponder);
            //mItemAdapterOption.reFill(mParameterMananer.getCompleteParameterList(mParameterMananer.getCurrentListType(), ItemListView.ITEM_SATALLITE.equals(mCurrentListType) ? mParameterMananer.getCurrentSatelliteIndex() : mParameterMananer.getCurrentTransponderIndex()));
            //mListViewOption.cleanChoosed();
            //mOptionTitleItemTextView.setText(mParameterMananer.getParameterListTitle(mParameterMananer.getCurrentListType(), ParameterMananer.ITEM_TRANSPONDER.equals(mParameterMananer.getCurrentListType()) ? mParameterMananer.getCurrentSatelliteIndex() : mParameterMananer.getCurrentTransponderIndex())/*"Ku_NewSat2"*/);
        }
    };

    /*public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                    if (selectedPosition == 0)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (selectedPosition == getAdapter().getCount() - 1)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    if (mListViewItem.requestFocus()) {
                        return true;
                    }
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if (mListViewOption.requestFocus())
                        return true;
                    break;
            }

            View selectedView = getSelectedView();
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if ( selectedView != null) {
                        clearChoosed(selectedView);
                    }
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if ( selectedView != null) {
                        setItemTextColor(selectedView, false);
                    }
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }*/
}

