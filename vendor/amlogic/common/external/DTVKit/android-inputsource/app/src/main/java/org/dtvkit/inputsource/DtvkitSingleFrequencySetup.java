package org.dtvkit.inputsource;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.text.Editable;
import android.text.TextUtils;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.view.KeyEvent;
import android.widget.Toast;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.WindowManager;
import android.util.TypedValue;
import android.os.Handler;

import org.dtvkit.companionlibrary.EpgSyncJobService;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Locale;
import java.util.ArrayList;
import java.util.List;

import com.droidlogic.fragment.ParameterMananer;
import com.droidlogic.settings.ConstantManager;
import org.droidlogic.dtvkit.DtvkitGlueClient;
import org.dtvkit.companionlibrary.model.Channel;

public class DtvkitSingleFrequencySetup {
    private static final String TAG = "DtvkitSingleFrequencySetup";
    private static final boolean DEBUG = true;

    private DataMananer mDataMananer;
    private boolean mStartSync = false;
    private boolean mStartSearch = false;
    private JSONArray mServiceList = null;
    private int mFoundServiceNumber = 0;

    private Context mContext;
    private boolean mIsDvbt = false;
    private boolean mIsDvbt2 = false;
    private int mFrequency = -1;//hz
    private String mInputId;
    private SingleFrequencyCallback mSingleFrequencyCallback;
    private Handler mMainHandler = new Handler();

    public static final String SINGLE_FREQUENCY_STATUS_ITEM = "status_item";
    public static final String SINGLE_FREQUENCY_STATUS_SEARCH_START = "search_start";
    public static final String SINGLE_FREQUENCY_STATUS_SEARCH_PROGRESS = "search_progress";
    public static final String SINGLE_FREQUENCY_STATUS_SEARCH_CHANNEL_NUMBER = "search_channel_number";
    public static final String SINGLE_FREQUENCY_STATUS_SEARCH_FINISH = "search_finish";
    public static final String SINGLE_FREQUENCY_STATUS_SAVE_START = "save_start";
    public static final String SINGLE_FREQUENCY_STATUS_SAVE_FINISH = "save_finish";
    public static final String SINGLE_FREQUENCY_CHANNEL_NAME = "channel_name";
    public static final String SINGLE_FREQUENCY_CHANNEL_DISPLAY_NUNMBER = "display_number";

    private final DtvkitGlueClient.SignalHandler mHandler = new DtvkitGlueClient.SignalHandler() {
        @Override
        public void onSignal(final String signal, final JSONObject data) {
            mMainHandler.post(new Runnable() {
                public void run() {
                    responseOnSignal(signal, data);
                }
            });
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, final Intent intent) {
            mMainHandler.post(new Runnable() {
                public void run() {
                    responseOnReceive(intent);
                }
            });
        }
    };

    public DtvkitSingleFrequencySetup(Context context, String channelSignalType, int frequency, String inputId, SingleFrequencyCallback callback) {
        mContext = context;
        mIsDvbt = !TextUtils.equals(channelSignalType, Channel.FIXED_SIGNAL_TYPE_DVBC);
        mIsDvbt2 = TextUtils.equals(channelSignalType, Channel.FIXED_SIGNAL_TYPE_DVBT2);
        mFrequency = frequency;
        mInputId = inputId;
        mSingleFrequencyCallback = callback;
        mDataMananer = new DataMananer(context);
    }

    private JSONArray initSearchParameter() {
        JSONArray args = new JSONArray();
        args.put(true);// retune
        if (mFrequency != -1) {
            if (mIsDvbt) {
                args.put(false);//nit
                args.put(mFrequency);//hz
                args.put(DataMananer.VALUE_DVBT_BANDWIDTH_LIST[mDataMananer.getIntParameters(DataMananer.KEY_DVBT_BANDWIDTH)]);
                args.put(DataMananer.VALUE_DVBT_MODE_LIST[mDataMananer.getIntParameters(DataMananer.KEY_DVBT_MODE)]);
                args.put(mIsDvbt2 ? DataMananer.VALUE_DVBT_TYPE_LIST[1] : DataMananer.VALUE_DVBT_TYPE_LIST[0]);
            } else {
                args.put(false);//nit
                args.put(mFrequency);//hz
                args.put(DataMananer.VALUE_DVBC_MODE_LIST[mDataMananer.getIntParameters(DataMananer.KEY_DVBC_MODE)]);
                args.put(mDataMananer.getIntParameters(DataMananer.KEY_DVBC_SYMBOL_RATE));
            }
            return args;
        } else {
            return null;
        }
    }

    public void startSearch() {
        startMonitoringSearch();
        mFoundServiceNumber = 0;
        try {
            JSONArray args = new JSONArray();
            args.put(false); // Commit
            DtvkitGlueClient.getInstance().request(mIsDvbt ? "Dvbt.finishSearch" : "Dvbc.finishSearch", args);
        } catch (Exception e) {
            Log.i(TAG, "startSearch Failed to finish search " + e.getMessage());
            return;
        }

        try {
            JSONArray args = initSearchParameter();
            if (args != null) {
                String command = (mIsDvbt ? "Dvbt.startManualSearchByFreq" : "Dvbc.startManualSearchByFreq");
                Log.d(TAG, "startSearch command = " + command + ", args = " + args.toString());
                DtvkitGlueClient.getInstance().request(command, args);
                mStartSearch = true;
            } else {
                stopSearch();
            }
        } catch (Exception e) {
            Log.i(TAG, "startSearch search Exception " + e.getMessage());
            stopSearch();
        }
    }

    public void stopSearch() {
        mStartSearch = false;
        stopMonitoringSearch();
        try {
            JSONArray args = new JSONArray();
            args.put(true); // Commit
            DtvkitGlueClient.getInstance().request(mIsDvbt ? "Dvbt.finishSearch" : "Dvbc.finishSearch", args);
        } catch (Exception e) {
            Log.i(TAG, "stopSearch failed to finish search Exception " + e.getMessage());
            return;
        }
    }

    private void onSearchFinished() {
        mStartSearch = false;
        stopMonitoringSearch();
        try {
            JSONArray args = new JSONArray();
            args.put(true); // Commit
            DtvkitGlueClient.getInstance().request(mIsDvbt ? "Dvbt.finishSearch" : "Dvbc.finishSearch", args);
        } catch (Exception e) {
            Log.i(TAG, "onSearchFinished failed to finish search Exception " + e.getMessage());
            return;
        }
        //update search results as After the search is finished, the lcn will be reordered
        mServiceList = getServiceList();
        mFoundServiceNumber = getFoundServiceNumber();
        if (mFoundServiceNumber == 0 && mServiceList != null && mServiceList.length() > 0) {
            Log.d(TAG, "mFoundServiceNumber erro use mServiceList length = " + mServiceList.length());
            mFoundServiceNumber = mServiceList.length();
        }
        startMonitoringSync();
        // By default, gets all channels and 1 hour of programs (DEFAULT_IMMEDIATE_EPG_DURATION_MILLIS)
        EpgSyncJobService.cancelAllSyncRequests(mContext);

        // If the intent that started this activity is from Live Channels app
        EpgSyncJobService.requestImmediateSyncSearchedChannel(mContext, mInputId, (mFoundServiceNumber > 0),new ComponentName(mContext, DtvkitEpgSync.class));
    }

    private void startMonitoringSearch() {
        DtvkitGlueClient.getInstance().registerSignalHandler(mHandler);
        if (mSingleFrequencyCallback != null) {
            try {
                JSONObject mess = new JSONObject();
                mess.put(SINGLE_FREQUENCY_STATUS_ITEM, SINGLE_FREQUENCY_STATUS_SEARCH_START);
                mSingleFrequencyCallback.onMessageCallback(mess);
                Log.i(TAG, "startMonitoringSearch " + mess.toString());
            } catch (JSONException e) {
                Log.i(TAG, "startMonitoringSearch JSONException " + e.getMessage());
            }
        }
    }

    private void stopMonitoringSearch() {
        DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
        if (mSingleFrequencyCallback != null) {
            try {
                JSONObject mess = new JSONObject();
                mess.put(SINGLE_FREQUENCY_STATUS_ITEM, SINGLE_FREQUENCY_STATUS_SEARCH_FINISH);
                mSingleFrequencyCallback.onMessageCallback(mess);
                Log.i(TAG, "stopMonitoringSearch " + mess.toString());
            } catch (JSONException e) {
                Log.i(TAG, "stopMonitoringSearch JSONException " + e.getMessage());
            }
        }
    }

    private void startMonitoringSync() {
        mStartSync = true;
        LocalBroadcastManager.getInstance(mContext).registerReceiver(mReceiver,
                new IntentFilter(EpgSyncJobService.ACTION_SYNC_STATUS_CHANGED));
        if (mSingleFrequencyCallback != null) {
            try {
                JSONObject mess = new JSONObject();
                mess.put(SINGLE_FREQUENCY_STATUS_ITEM, SINGLE_FREQUENCY_STATUS_SAVE_START);
                mSingleFrequencyCallback.onMessageCallback(mess);
                Log.i(TAG, "startMonitoringSync " + mess.toString());
            } catch (JSONException e) {
                Log.i(TAG, "startMonitoringSync JSONException " + e.getMessage());
            }
        }
    }

    private void stopMonitoringSync() {
        mStartSync = false;
        LocalBroadcastManager.getInstance(mContext).unregisterReceiver(mReceiver);
        if (mSingleFrequencyCallback != null) {
            String firstServiceName = "";
            String firstServiceDisplayNumber = "";
            try {
                firstServiceName = (mServiceList != null && mServiceList.length() > 0) ? mServiceList.getJSONObject(0).getString("name") : "";
                firstServiceDisplayNumber = (mServiceList != null && mServiceList.length() > 0) ? String.format(Locale.ENGLISH, "%d", mServiceList.getJSONObject(0).getInt("lcn")) : "";
            } catch (JSONException e) {
                Log.e(TAG, "stopMonitoringSync firstService JSONException = " + e.getMessage());
            }
            try {
                JSONObject mess = new JSONObject();
                mess.put(SINGLE_FREQUENCY_STATUS_ITEM, SINGLE_FREQUENCY_STATUS_SAVE_FINISH);
                mess.put(SINGLE_FREQUENCY_CHANNEL_NAME, firstServiceName);
                mess.put(SINGLE_FREQUENCY_CHANNEL_DISPLAY_NUNMBER, firstServiceDisplayNumber);
                mSingleFrequencyCallback.onMessageCallback(mess);
                Log.i(TAG, "stopMonitoringSync onMessageCallback " + mess.toString());
            } catch (JSONException e) {
                Log.i(TAG, "stopMonitoringSync JSONException " + e.getMessage());
            }
        }
    }

    private int getFoundServiceNumber() {
        int found = 0;
        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getNumberOfServices", new JSONArray());
            found = obj.getInt("data");
            Log.i(TAG, "getFoundServiceNumber found = " + found);
        } catch (Exception ignore) {
            Log.i(TAG, "getFoundServiceNumber Exception = " + ignore.getMessage());
        }
        return found;
    }

    private int getSearchProcess(JSONObject data) {
        int progress = 0;
        if (data == null) {
            return progress;
        }
        try {
            progress = data.getInt("progress");
        } catch (JSONException ignore) {
            Log.i(TAG, "getSearchProcess Exception = " + ignore.getMessage());
        }
        return progress;
    }

    private JSONArray getServiceList() {
        JSONArray result = null;
        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getListOfServices", new JSONArray());
            JSONArray services = obj.getJSONArray("data");
            result = services;
            for (int i = 0; i < services.length(); i++) {
                JSONObject service = services.getJSONObject(i);
                if (DEBUG) {
                    Log.i(TAG, "getServiceList service = " + service.toString());
                }
            }
        } catch (Exception e) {
            Log.i(TAG, "getServiceList Exception = " + e.getMessage());
        }
        return result;
    }

    private void responseOnSignal(String signal, JSONObject data) {
        if ((mIsDvbt && signal.equals("DvbtStatusChanged")) || (!mIsDvbt && signal.equals("DvbcStatusChanged"))) {
            int progress = getSearchProcess(data);
            if (progress < 100) {
                Log.d(TAG, "onSignal progress = " + progress);
                if (mSingleFrequencyCallback != null) {
                    try {
                        JSONObject mess = new JSONObject();
                        mess.put(SINGLE_FREQUENCY_STATUS_ITEM, SINGLE_FREQUENCY_STATUS_SEARCH_PROGRESS);
                        mess.put(SINGLE_FREQUENCY_STATUS_SEARCH_PROGRESS, progress);
                        mSingleFrequencyCallback.onMessageCallback(mess);
                        Log.i(TAG, "onSignal search progress " + mess.toString());
                    } catch (JSONException e) {
                        Log.i(TAG, "onSignal JSONException1 " + e.getMessage());
                    }
                }
            } else {
                Log.d(TAG, "onSignal search finished");
                onSearchFinished();
            }
        }
    }

    private void responseOnReceive(Intent intent) {
        if (intent != null) {
            String status = intent.getStringExtra(EpgSyncJobService.SYNC_STATUS);
            if (status.equals(EpgSyncJobService.SYNC_FINISHED)) {
                mStartSync = false;
                stopMonitoringSync();
            }
        } else {
            Log.d(TAG, "responseOnReceive null");
        }
    }

    public interface SingleFrequencyCallback {
        void onMessageCallback(JSONObject mess);
    }
}
