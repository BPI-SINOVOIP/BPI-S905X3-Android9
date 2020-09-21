package org.dtvkit.inputsource;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.RadioGroup;
import android.widget.Button;
import android.media.tv.TvInputInfo;

import com.droidlogic.settings.ConstantManager;

public class DtvkitDvbScanSelect extends Activity {
    private static final String TAG = "DtvkitDvbScanSelect";

    private static final int[] RES = {R.id.select_dvbc, R.id.select_dvbt, R.id.select_dvbs};

    public static final int REQUEST_CODE_START_DVBC_ACTIVITY = 1;
    public static final int REQUEST_CODE_START_DVBT_ACTIVITY = 2;
    public static final int REQUEST_CODE_START_DVBS_ACTIVITY = 3;
    public static final int REQUEST_CODE_START_SETTINGS_ACTIVITY = 4;

    public static final int SEARCH_TYPE_MANUAL = 0;
    public static final int SEARCH_TYPE_AUTO = 1;
    public static final int SEARCH_TYPE_DVBC = 0;
    public static final int SEARCH_TYPE_DVBT = 1;
    public static final int SEARCH_TYPE_DVBS = 2;
    public static final String SEARCH_TYPE_MANUAL_AUTO = "search_manual_auto";
    public static final String SEARCH_TYPE_DVBS_DVBT_DVBC = "search_dvbs_dvbt_dvbc";
    public static final String SEARCH_FOUND_SERVICE_NUMBER = "service_number";
    public static final String SEARCH_FOUND_SERVICE_LIST = "service_list";
    public static final String SEARCH_FOUND_SERVICE_INFO = "service_info";
    public static final String SEARCH_FOUND_FIRST_SERVICE = "first_service";

    private DataMananer mDataMananer;
    private Intent mIntent = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.select_search_activity);
        mIntent = getIntent();
        mDataMananer = new DataMananer(this);
        initLayout();
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_CODE_START_DVBC_ACTIVITY:
            case REQUEST_CODE_START_DVBT_ACTIVITY:
            case REQUEST_CODE_START_DVBS_ACTIVITY:
            case REQUEST_CODE_START_SETTINGS_ACTIVITY:
                if (resultCode == RESULT_OK) {
                    setResult(RESULT_OK, data);
                    finish();
                } else {
                    setResult(RESULT_CANCELED);
                }
                break;
            default:
                // do nothing
                Log.d(TAG, "onActivityResult other request");
        }
    }

    private void initLayout() {
        int index = mDataMananer.getIntParameters(DataMananer.KEY_SELECT_SEARCH_ACTIVITY);
        Button dvbc = (Button)findViewById(R.id.select_dvbc);
        final Intent intent = new Intent();
        final String inputId = mIntent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        final String pvrStatus = mIntent.getStringExtra(ConstantManager.KEY_LIVETV_PVR_STATUS);
        if (inputId != null) {
            intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
        }
        //init pvr set flag when inited
        String firstPvrFlag = PvrStatusConfirmManager.read(this, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG);
        if (!PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG_FIRST.equals(firstPvrFlag)) {
            PvrStatusConfirmManager.store(this, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG_FIRST);
        }
        dvbc.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String pvrFlag = PvrStatusConfirmManager.read(DtvkitDvbScanSelect.this, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG);
                if (pvrStatus != null && PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG_FIRST.equals(pvrFlag)) {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, pvrStatus);
                } else {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, "");
                }
                intent.setClassName(DataMananer.KEY_PACKAGE_NAME, DataMananer.KEY_ACTIVITY_DVBT);
                intent.putExtra(DataMananer.KEY_IS_DVBT, false);
                startActivityForResult(intent, REQUEST_CODE_START_DVBC_ACTIVITY);
                mDataMananer.saveIntParameters(DataMananer.KEY_SELECT_SEARCH_ACTIVITY, DataMananer.SELECT_DVBC);
                Log.d(TAG, "select_dvbc inputId = " + inputId);
            }
        });
        Button dvbt = (Button)findViewById(R.id.select_dvbt);
        dvbt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String pvrFlag = PvrStatusConfirmManager.read(DtvkitDvbScanSelect.this, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG);
                if (pvrStatus != null && PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG_FIRST.equals(pvrFlag)) {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, pvrStatus);
                } else {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, "");
                }
                intent.setClassName(DataMananer.KEY_PACKAGE_NAME, DataMananer.KEY_ACTIVITY_DVBT);
                intent.putExtra(DataMananer.KEY_IS_DVBT, true);
                startActivityForResult(intent, REQUEST_CODE_START_DVBT_ACTIVITY);
                mDataMananer.saveIntParameters(DataMananer.KEY_SELECT_SEARCH_ACTIVITY, DataMananer.SELECT_DVBT);
                Log.d(TAG, "select_dvbt inputId = " + inputId);
            }
        });
        Button dvbs = (Button)findViewById(R.id.select_dvbs);
        dvbs.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String pvrFlag = PvrStatusConfirmManager.read(DtvkitDvbScanSelect.this, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG);
                if (pvrStatus != null && PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG_FIRST.equals(pvrFlag)) {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, pvrStatus);
                } else {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, "");
                }
                intent.setClassName(DataMananer.KEY_PACKAGE_NAME, DataMananer.KEY_ACTIVITY_DVBS);
                startActivityForResult(intent, REQUEST_CODE_START_DVBS_ACTIVITY);
                mDataMananer.saveIntParameters(DataMananer.KEY_SELECT_SEARCH_ACTIVITY, DataMananer.SELECT_DVBS);
                Log.d(TAG, "select_dvbs inputId = " + inputId);
            }
        });
        Button settings = (Button)findViewById(R.id.select_setting);
        settings.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String pvrFlag = PvrStatusConfirmManager.read(DtvkitDvbScanSelect.this, PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG);
                if (pvrStatus != null && PvrStatusConfirmManager.KEY_PVR_CLEAR_FLAG_FIRST.equals(pvrFlag)) {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, pvrStatus);
                } else {
                    intent.putExtra(ConstantManager.KEY_LIVETV_PVR_STATUS, "");
                }
                intent.setClassName(DataMananer.KEY_PACKAGE_NAME, DataMananer.KEY_ACTIVITY_SETTINGS);
                startActivityForResult(intent, REQUEST_CODE_START_SETTINGS_ACTIVITY);
                mDataMananer.saveIntParameters(DataMananer.KEY_SELECT_SEARCH_ACTIVITY, DataMananer.SELECT_SETTINGS);
                Log.d(TAG, "start to set related language");
            }
        });
        switch (index) {
            case DataMananer.SELECT_DVBC:
                dvbc.requestFocus();
                break;
            case DataMananer.SELECT_DVBT:
                dvbt.requestFocus();
                break;
            case DataMananer.SELECT_DVBS:
                dvbs.requestFocus();
                break;
            case DataMananer.SELECT_SETTINGS:
                settings.requestFocus();
                break;
            default:
                dvbs.requestFocus();
                break;
        }
    }
}
