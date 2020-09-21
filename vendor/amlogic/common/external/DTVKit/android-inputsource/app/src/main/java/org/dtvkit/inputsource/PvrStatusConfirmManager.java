package org.dtvkit.inputsource;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.content.DialogInterface;
import android.view.WindowManager;
import android.text.TextUtils;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;


import com.droidlogic.settings.ConstantManager;

public class PvrStatusConfirmManager {
    public static final String TAG = PvrStatusConfirmManager.class.getSimpleName();

    private Context mContext = null;
    private String mPvrStatus = null;
    private String mPvrDeleteResponse = null;
    private Callback mCallback = null;
    private DataMananer mDataMananer;
    private String mSearchType = ConstantManager.KEY_DTVKIT_SEARCH_TYPE_MANUAL;

    public static final String KEY_PVR_CLEAR_FLAG = "pvr_clear_flag";
    public static final String KEY_PVR_CLEAR_FLAG_FIRST = "first";
    public static final String KEY_PVR_CLEAR_FLAG_SECOND = "second";

    private final BroadcastReceiver mDvrCommandReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "intent = " + (intent != null ? intent.toString() : "null"));
            if (intent != null) {
                if (ConstantManager.ACTION_DVR_RESPONSE.equals(intent.getAction())) {
                    mPvrDeleteResponse = intent.getStringExtra(ConstantManager.ACTION_DVR_RESPONSE);
                    if (mCallback != null) {
                        mCallback.onStatusCallback(mPvrDeleteResponse);
                    }
                }
            }
        }
    };

    private interface Callback {
        void onStatusCallback(String value);
    }

    public PvrStatusConfirmManager(Context context, DataMananer dataMananer) {
        mContext = context;
        mDataMananer = dataMananer;
    }

    public void setPvrStatus(String status) {
        mPvrStatus = status;
    }

    public void setSearchType(String type) {
        mSearchType = type;
    }

    private boolean isScheduleRecordingAvailable() {
        boolean result = false;
        if (mPvrStatus != null) {
            String[] split = mPvrStatus.split(",");
            if (split != null && split.length > 0) {
                for (String temp : split) {
                    if ("schedule".equals(temp)) {
                        result = true;
                    }
                }
            }
        }
        return result;
    }

    private boolean isRecordProgramAvailable() {
        boolean result = false;
        if (mPvrStatus != null) {
            String[] split = mPvrStatus.split(",");
            if (split != null && split.length > 0) {
                for (String temp : split) {
                    if ("program".equals(temp)) {
                        result = true;
                    }
                }
            }
        }
        return result;
    }

    public boolean needDeletePvrRecordings() {
        boolean result = false;
        result = isScheduleRecordingAvailable();
        //check not started and in progress schedules only
        /*if (ConstantManager.KEY_DTVKIT_SEARCH_TYPE_AUTO.equals(mSearchType)) {
            result = result || isRecordProgramAvailable();
        }*/
        return result;
    }

    public void registerCommandReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ConstantManager.ACTION_DVR_RESPONSE);
        mContext.registerReceiver(mDvrCommandReceiver, intentFilter);
    }

    public void unRegisterCommandReceiver() {
        mContext.unregisterReceiver(mDvrCommandReceiver);
    }

    public void showDialogToAppoint(final Context context, final boolean autoSearch) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog alert = builder.create();
        final View dialogView = View.inflate(context, R.layout.pvr_record_delete_confirm, null);
        final TextView textview = (TextView) dialogView.findViewById(R.id.dialog_title);
        final Button delete = (Button) dialogView.findViewById(R.id.dialog_delete);
        final Button cancel = (Button) dialogView.findViewById(R.id.dialog_cancel);
        delete.requestFocus();
        delete.setEnabled(true);
        cancel.setEnabled(true);
        if (autoSearch) {
            textview.setText(R.string.pvr_record_confirm_delete_schedule);
        } else {
            textview.setText(R.string.pvr_record_confirm_stop_record);
        }
        mCallback = new Callback(){
            @Override
            public void onStatusCallback(String value) {
                final String[] STATUS = {"notstarted", "started", "failed", "recorded"};
                boolean responsed = false;
                int count = 0;
                if (!TextUtils.isEmpty(value)) {
                    String[] split = value.split(",");
                    if (split != null && split.length > 0) {
                        for (String one : split) {
                            boolean changed = false;
                            for (String two : STATUS) {
                                if (two.equals(one)) {
                                    Log.d(TAG, "onStatusCallback " + two);
                                    changed = true;
                                    count++;
                                }
                            }
                            if (!changed) {
                                Log.d(TAG, "onStatusCallback unkown");
                            }
                        }
                        if (count > 0) {
                            responsed = true;
                        }
                    }
                    if (responsed) {
                        mPvrStatus = null;
                        if (textview != null) {
                            if (autoSearch) {
                                textview.setText(R.string.pvr_record_confirm_delete_done);
                            } else {
                                textview.setText(R.string.pvr_record_confirm_stop_done);
                            }
                        }
                        if (delete != null) {
                            delete.setEnabled(false);
                        }
                        if (cancel != null) {
                            cancel.requestFocus();
                        }
                        if (alert != null) {
                            //alert.dismiss();
                        }
                    } else {
                        Log.d(TAG, "onStatusCallback unresponsed");
                    }
                } else  {
                    Log.d(TAG, "onStatusCallback on need to delete");
                    mPvrStatus = null;
                    if (textview != null) {
                        if (autoSearch) {
                            textview.setText(R.string.pvr_record_confirm_delete_done);
                        } else {
                            textview.setText(R.string.pvr_record_confirm_stop_done);
                        }
                    }
                    if (delete != null) {
                        delete.setEnabled(false);
                    }
                    if (cancel != null) {
                        cancel.requestFocus();
                    }
                    if (alert != null) {
                        //alert.dismiss();
                    }
                }
            }
        };

        delete.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "delete onClick");
                sendDvrCommand(context);
                textview.setText(R.string.pvr_record_confirm_deleting);
                //alert.dismiss();
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "exit onClick");
                alert.dismiss();
            }
        });
        alert.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "onDismiss");
                mCallback = null;
            }
        });
        alert.setView(dialogView);
        alert.show();
        //set size and background
        WindowManager.LayoutParams params = alert.getWindow().getAttributes();
        params.width = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 400, context.getResources().getDisplayMetrics());
        params.height = WindowManager.LayoutParams.WRAP_CONTENT ;
        alert.getWindow().setAttributes(params);
    }

    public void sendDvrCommand(Context context) {
        Log.d(TAG, "sendDvrCommand");
        String searchType = ConstantManager.KEY_DTVKIT_SEARCH_TYPE_MANUAL;
        //delete not started and in progress schedule for the moment
        /*if (ConstantManager.KEY_DTVKIT_SEARCH_TYPE_AUTO.equals(mSearchType)) {
            searchType = ConstantManager.KEY_DTVKIT_SEARCH_TYPE_AUTO;
        }*/
        Intent intent = new Intent(ConstantManager.ACTION_REMOVE_ALL_DVR_RECORDS);
        intent.putExtra(ConstantManager.KEY_DTVKIT_SEARCH_TYPE, searchType);
        if (context != null) {
            context.sendBroadcast(intent);
            //modify pvr set flag after send command
            String secondPvrFlag = read(context, KEY_PVR_CLEAR_FLAG);
            if (!KEY_PVR_CLEAR_FLAG_SECOND.equals(secondPvrFlag)) {
                store(context, KEY_PVR_CLEAR_FLAG, KEY_PVR_CLEAR_FLAG_SECOND);
            }
        } else {
            Log.d(TAG, "sendDvrCommand null context");
        }
    }

    public static void store(Context context, String keyword, String content) {
        SharedPreferences DealData = context.getSharedPreferences("pvr_info", 0);
        Editor editor = DealData.edit();
        editor.putString(keyword, content);
        editor.commit();
        Log.d(TAG, "store keyword: " + keyword + ",content: " + content);
    }

    public static String read(Context context, String keyword) {
        SharedPreferences DealData = context.getSharedPreferences("pvr_info", 0);
        Log.d(TAG, "read keyword: " + keyword + ",value: " + DealData.getString(keyword, null));
        return DealData.getString(keyword, null);
    }
}
