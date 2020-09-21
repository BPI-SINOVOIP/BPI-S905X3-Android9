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
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.content.ActivityNotFoundException;
import android.widget.Toast;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.LinearLayout;
import android.text.Editable;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.widget.Toast;

import org.dtvkit.companionlibrary.EpgSyncJobService;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.droidlogic.dtvkit.DtvkitGlueClient;

import com.droidlogic.settings.ConstantManager;
import com.droidlogic.fragment.ParameterMananer;

import java.util.Locale;

public class DtvkitDvbsSetup extends Activity {
    private static final String TAG = "DtvkitDvbsSetup";
    private static final int REQUEST_CODE_SET_UP_SETTINGS = 1;
    private DataMananer mDataMananer;
    private boolean mStartSync = false;
    private boolean mStartSearch = false;
    private int mFoundServiceNumber = 0;
    private JSONArray mServiceList = null;
    private int mSearchType = -1;// 0 manual 1 auto
    private int mSearchDvbsType = -1;
    private boolean mSetUp = false;
    private PvrStatusConfirmManager mPvrStatusConfirmManager = null;
    private ParameterMananer mParameterMananer;

    private final DtvkitGlueClient.SignalHandler mHandler = new DtvkitGlueClient.SignalHandler() {
        @Override
        public void onSignal(String signal, JSONObject data) {
            if (signal.equals("DvbsStatusChanged")) {
                int progress = getSearchProcess(data);
                Log.d(TAG, "onSignal progress = " + progress);
                if (progress < 100) {
                    getProgressBar().setProgress(progress);
                    setSearchStatus(String.format(Locale.ENGLISH, "Searching (%d%%)", progress));
                } else {
                    onSearchFinished();
                }
            }
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
       @Override
       public void onReceive(Context context, final Intent intent)
       {
          String status = intent.getStringExtra(EpgSyncJobService.SYNC_STATUS);
          if (status.equals(EpgSyncJobService.SYNC_FINISHED))
          {
             setSearchStatus("Finished");
             mStartSync = false;
             finish();
          }
       }
    };

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mStartSync) {
            Toast.makeText(DtvkitDvbsSetup.this, R.string.sync_tv_provider, Toast.LENGTH_SHORT).show();
            return true;
        }
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mStartSearch) {
                onSearchFinished();
                return true;
            } else {
                stopMonitoringSearch();
                stopSearch(false);
                finish();
                return true;
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.satsetup);

        mDataMananer = new DataMananer(this);
        mPvrStatusConfirmManager = new PvrStatusConfirmManager(this, mDataMananer);
        mParameterMananer = new ParameterMananer(this, DtvkitGlueClient.getInstance());
        Intent intent = getIntent();
        if (intent != null) {
            String status = intent.getStringExtra(ConstantManager.KEY_LIVETV_PVR_STATUS);
            mPvrStatusConfirmManager.setPvrStatus(status);
            Log.d(TAG, "onCreate pvr status = " + status);
        }

        final Button search = (Button)findViewById(R.id.startsearch);
        final Button stop = (Button)findViewById(R.id.stopsearch);
        final Button setup = (Button)findViewById(R.id.setup);
        final Button importSatellite = (Button)findViewById(R.id.import_satellite);
        search.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mPvrStatusConfirmManager.setSearchType(ConstantManager.KEY_DTVKIT_SEARCH_TYPE_MANUAL);
                boolean checkPvr = mPvrStatusConfirmManager.needDeletePvrRecordings();
                if (checkPvr) {
                    mPvrStatusConfirmManager.showDialogToAppoint(DtvkitDvbsSetup.this, false);
                } else {
                    mPvrStatusConfirmManager.sendDvrCommand(DtvkitDvbsSetup.this);
                    search.setEnabled(false);
                    stop.setEnabled(true);
                    stop.requestFocus();
                    startSearch();
                }
            }
        });
        search.requestFocus();

        stop.setEnabled(false);
        stop.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                stop.setEnabled(false);
                search.setEnabled(true);
                search.requestFocus();
                stopSearch(true);
            }
        });

        setup.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mPvrStatusConfirmManager.setSearchType(ConstantManager.KEY_DTVKIT_SEARCH_TYPE_MANUAL);
                boolean checkPvr = mPvrStatusConfirmManager.needDeletePvrRecordings();
                if (checkPvr) {
                    mPvrStatusConfirmManager.showDialogToAppoint(DtvkitDvbsSetup.this, false);
                } else {
                    mPvrStatusConfirmManager.sendDvrCommand(DtvkitDvbsSetup.this);
                    setUp();
                }
            }
        });

        //ui set visibility gone as will be excuted after first boot
        importSatellite.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mParameterMananer.importDatabase(ConstantManager.DTVKIT_SATELLITE_DATA);
            }
        });

        CheckBox nit = (CheckBox)findViewById(R.id.network);
        CheckBox clear = (CheckBox)findViewById(R.id.clear_old);
        CheckBox dvbs2 = (CheckBox)findViewById(R.id.dvbs2);
        Spinner searchmode = (Spinner)findViewById(R.id.search_mode);
        Spinner fecmode = (Spinner)findViewById(R.id.fec_mode);
        Spinner modulationmode = (Spinner)findViewById(R.id.modulation_mode);
        final LinearLayout blindContainer = (LinearLayout)findViewById(R.id.blind_frequency_container);
        EditText edit_start_freq = (EditText)findViewById(R.id.edit_start_freq);
        EditText edit_end_freq = (EditText)findViewById(R.id.edit_end_freq);
        edit_start_freq.setText(DataMananer.VALUE_BLIND_DEFAULT_START_FREQUENCY + "");
        edit_end_freq.setText(DataMananer.VALUE_BLIND_DEFAULT_END_FREQUENCY + "");

        nit.setChecked(mDataMananer.getIntParameters(DataMananer.KEY_DVBS_NIT) == 1 ? true : false);
        clear.setChecked(mDataMananer.getIntParameters(DataMananer.KEY_CLEAR) == 1 ? true : false);
        dvbs2.setChecked(mDataMananer.getIntParameters(DataMananer.KEY_DVBS2) == 1 ? true : false);
        nit.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (nit.isChecked()) {
                    mDataMananer.saveIntParameters(DataMananer.KEY_DVBS_NIT, 1);
                } else {
                    mDataMananer.saveIntParameters(DataMananer.KEY_DVBS_NIT, 0);
                }
            }
        });
        clear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (clear.isChecked()) {
                    mDataMananer.saveIntParameters(DataMananer.KEY_CLEAR, 1);
                } else {
                    mDataMananer.saveIntParameters(DataMananer.KEY_CLEAR, 0);
                }
            }
        });
        dvbs2.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (dvbs2.isChecked()) {
                    mDataMananer.saveIntParameters(DataMananer.KEY_DVBS2, 1);
                } else {
                    mDataMananer.saveIntParameters(DataMananer.KEY_DVBS2, 0);
                }
            }
        });
        searchmode.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "searchmode onItemSelected position = " + position);
                mDataMananer.saveIntParameters(DataMananer.KEY_SEARCH_MODE, position);
                //no need to add limits now
                /*if (position == DataMananer.VALUE_SEARCH_MODE_BLIND) {
                    blindContainer.setVisibility(View.VISIBLE);
                    nit.setVisibility(View.GONE);
                    mDataMananer.saveStringParameters(DataMananer.KEY_TRANSPONDER, "null");
                } else {
                    blindContainer.setVisibility(View.GONE);
                    nit.setVisibility(View.VISIBLE);
                }*/
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        fecmode.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "fecmode onItemSelected position = " + position);
                mDataMananer.saveIntParameters(DataMananer.KEY_FEC_MODE, position);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        modulationmode.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "modulationmode onItemSelected position = " + position);
                mDataMananer.saveIntParameters(DataMananer.KEY_MODULATION_MODE, position);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        int searchmodeValue = mDataMananer.getIntParameters(DataMananer.KEY_SEARCH_MODE);
        searchmode.setSelection(searchmodeValue);
        //no need to add limits now
        /*if (searchmodeValue == DataMananer.VALUE_SEARCH_MODE_BLIND) {
            blindContainer.setVisibility(View.VISIBLE);
            nit.setVisibility(View.GONE);
        } else {
            blindContainer.setVisibility(View.GONE);
            nit.setVisibility(View.VISIBLE);
        }*/
        fecmode.setSelection(mDataMananer.getIntParameters(DataMananer.KEY_FEC_MODE));
        modulationmode.setSelection(mDataMananer.getIntParameters(DataMananer.KEY_MODULATION_MODE));
    }

    @Override
    public void finish() {
        //send search info to livetv if found any
        Log.d(TAG, "finish");
        if (mFoundServiceNumber > 0) {
            Intent intent = new Intent();
            intent.putExtra(DtvkitDvbScanSelect.SEARCH_TYPE_MANUAL_AUTO, mSearchType);
            intent.putExtra(DtvkitDvbScanSelect.SEARCH_TYPE_DVBS_DVBT_DVBC, mSearchDvbsType);
            intent.putExtra(DtvkitDvbScanSelect.SEARCH_FOUND_SERVICE_NUMBER, mFoundServiceNumber);
            intent.putExtra(DtvkitDvbScanSelect.SEARCH_FOUND_SERVICE_NUMBER, mFoundServiceNumber);
            String serviceListJsonArray = (mServiceList != null && mServiceList.length() > 0) ? mServiceList.toString() : "";
            String firstServiceName = "";
            try {
                firstServiceName = (mServiceList != null && mServiceList.length() > 0) ? mServiceList.getJSONObject(0).getString("name") : "";
            } catch (JSONException e) {
                Log.e(TAG, "finish JSONException = " + e.getMessage());
            }
            intent.putExtra(DtvkitDvbScanSelect.SEARCH_FOUND_SERVICE_LIST, serviceListJsonArray);
            intent.putExtra(DtvkitDvbScanSelect.SEARCH_FOUND_FIRST_SERVICE, firstServiceName);
            Log.d(TAG, "finish firstServiceName = " + firstServiceName);
            setResult(RESULT_OK, intent);
        } else {
            setResult(RESULT_CANCELED);
        }
        super.finish();
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        mSetUp = false;
        mPvrStatusConfirmManager.registerCommandReceiver();
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
        mPvrStatusConfirmManager.unRegisterCommandReceiver();
    }

    @Override
    public void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
        if (mSetUp) {//set up menu and no need to update
            return;
        }
        if (mStartSearch) {
            onSearchFinished();
        } else {
            stopMonitoringSearch();
            stopSearch(false);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
        stopMonitoringSearch();
        stopMonitoringSync();
    }

    private void setUp() {
        try {
            Intent intent = new Intent();
            intent.setClassName("org.dtvkit.inputsource", "com.droidlogic.fragment.ScanMainActivity");
            String inputId = this.getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
            if (inputId != null) {
                intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
            }
            startActivityForResult(intent, REQUEST_CODE_SET_UP_SETTINGS);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, getString(R.string.strSetUpNotFound), Toast.LENGTH_SHORT).show();
            return;
        }
        mSetUp = true;
    }

    private JSONArray initSearchParameter() {
        JSONArray args = new JSONArray();
        JSONObject obj = initLbnData();
        if (obj == null) {
            return null;
        }
        args.put(obj.toString());//arg1
        //needclear not needed
        //boolean needclear = mDataMananer.getIntParameters(DataMananer.KEY_CLEAR) == 1 ? true : false;
        //args.put(needclear);
        String searchmode = DataMananer.KEY_SEARCH_MODE_LIST[mDataMananer.getIntParameters(DataMananer.KEY_SEARCH_MODE)];
        args.put(mDataMananer.getIntParameters(DataMananer.KEY_DVBS2) == 1);//arg2
        args.put(DataMananer.KEY_MODULATION_ARRAY_VALUE[mDataMananer.getIntParameters(DataMananer.KEY_MODULATION_MODE)]);//arg3
        args.put(searchmode);//arg4
        switch (searchmode) {
            case "blind":
                Log.d(TAG, "initSearchParameter blind");
                args = initBlindSearch(args);
                break;
            case "satellite":
                Log.d(TAG, "initSearchParameter satellite");
                args = initSatelliteSearch(args);
                break;
            case "transponder":
                Log.d(TAG, "initSearchParameter transponder");
                args = initTransponderSearch(args);
                break;
            default:
                Log.d(TAG, "initSearchParameter not surported mode!");
                break;
        }
        return args;
    }

    private JSONArray initBlindSearch(JSONArray args) {
        try {
            //int[] result = getBlindFrequency();
            String satellite = mDataMananer.getStringParameters(DataMananer.KEY_SATALLITE);
            if (/*result[0] < 0 || result[1] < 0 || result[0] > result[1] || */TextUtils.isEmpty(satellite)) {
                return null;
            }
            //no need to add limits now
            //args.put(result[0]);// "start_freq" khz //arg5
            //args.put(result[1]);//"end_freq" khz //arg6
            args.put(satellite);//arg7
        } catch (Exception e) {
            args = null;
            Toast.makeText(this, R.string.dialog_parameter_set_blind, Toast.LENGTH_SHORT).show();
            Log.d(TAG, "initBlindSearch Exception " + e.getMessage());
        }
        return args;
    }

    private int[] getBlindFrequency() {
        int[] result = {-1, -1};
        EditText edit_start_freq = (EditText)findViewById(R.id.edit_start_freq);
        EditText edit_end_freq = (EditText)findViewById(R.id.edit_end_freq);
        Editable edit_start_freq_edit = edit_start_freq.getText();
        Editable edit_end_freq_edit = edit_end_freq.getText();
        if (edit_start_freq_edit != null && edit_end_freq_edit != null) {
            String edit_start_freq_value = edit_start_freq_edit.toString();
            String edit_end_freq_value = edit_end_freq_edit.toString();
            Log.d(TAG, "getBlindFrequency edit_start_freq_value = " + edit_start_freq_value +
                ", edit_end_freq_value = " + edit_end_freq_value);
            if (!TextUtils.isEmpty(edit_start_freq_value) && TextUtils.isDigitsOnly(edit_start_freq_value)) {
                result[0] = Integer.valueOf(edit_start_freq_value);
            }
            if (!TextUtils.isEmpty(edit_end_freq_value) && TextUtils.isDigitsOnly(edit_end_freq_value)) {
                result[1] = Integer.valueOf(edit_end_freq_value);
            }
        }
        return result;
    }

    private JSONArray initSatelliteSearch(JSONArray args) {
        try {
            args.put(mDataMananer.getIntParameters(DataMananer.KEY_DVBS_NIT) == 1 ? true : false);//arg5
            args.put(mDataMananer.getStringParameters(DataMananer.KEY_SATALLITE));//arg6
        } catch (Exception e) {
            args = null;
            Toast.makeText(this, R.string.dialog_parameter_select_satellite, Toast.LENGTH_SHORT).show();
            Log.d(TAG, "initSatelliteSearch Exception " + e.getMessage());
        }
        return args;
    }

    private JSONArray initTransponderSearch(JSONArray args) {
        try {
            args.put(mDataMananer.getIntParameters(DataMananer.KEY_DVBS_NIT) == 1);//arg5
            args.put(mDataMananer.getStringParameters(DataMananer.KEY_SATALLITE));//arg6
            String[] singleParameter = null;
            String parameter = mDataMananer.getStringParameters(DataMananer.KEY_TRANSPONDER);
            if (parameter != null) {
                singleParameter = parameter.split("/");
                if (singleParameter != null && singleParameter.length == 3) {
                    args.put(Integer.valueOf(singleParameter[0]));//arg7
                    args.put(singleParameter[1]);//arg8
                    args.put(Integer.valueOf(singleParameter[2]));//arg9
                } else {
                    Toast.makeText(this, R.string.dialog_parameter_select_transponer, Toast.LENGTH_SHORT).show();
                    return null;
                }
            } else {
                Toast.makeText(this, R.string.dialog_parameter_select_transponer, Toast.LENGTH_SHORT).show();
                return null;
            }
            args.put(DataMananer.KEY_FEC_ARRAY_VALUE[mDataMananer.getIntParameters(DataMananer.KEY_FEC_MODE)]);//arg10
        } catch (Exception e) {
            args = null;
            Log.d(TAG, "initTransponderSearch Exception " + e.getMessage());
        }
        return args;
    }

    private JSONObject initLbnData() {
        JSONObject obj = null;
        try {
            obj = new JSONObject();
            boolean unicable_switch = (mDataMananer.getIntParameters(DataMananer.KEY_UNICABLE_SWITCH) == 1);
            obj.put("unicable", unicable_switch);
            obj.put("unicable_chan", mDataMananer.getIntParameters(DataMananer.KEY_USER_BAND));
            obj.put("unicable_if", mDataMananer.getIntParameters(DataMananer.KEY_UB_FREQUENCY) * 1000);//mhz to khz
            obj.put("unicable_position_b", mDataMananer.getIntParameters(DataMananer.KEY_POSITION) == 1);
            obj.put("tone_burst", DataMananer.DIALOG_SET_SELECT_SINGLE_ITEM_TONE_BURST_LIST[mDataMananer.getIntParameters(DataMananer.KEY_TONE_BURST)]);
            obj.put("c_switch", mDataMananer.getIntParameters(DataMananer.KEY_DISEQC1_0));
            obj.put("u_switch", mDataMananer.getIntParameters(DataMananer.KEY_DISEQC1_1));
            obj.put("dish_pos", mDataMananer.getIntParameters(DataMananer.KEY_DISEQC1_2_DISH_CURRENT_POSITION));
            int lnb_type = mDataMananer.getIntParameters(DataMananer.KEY_LNB_TYPE);
            //saved lnbtype: 0:single, 1:universal, 2:user define
            //needed lnbtype: 0:single, 1:universal, 2:unicable, 3:user define
            if (unicable_switch) {
                lnb_type = 2;//unicable
            } else if (lnb_type == 2) {//
                lnb_type = 3;//user define
            }
            obj.put("lnb_type", lnb_type);

            JSONObject lnbobj = new JSONObject();
            JSONObject lowband_obj = new JSONObject();
            JSONObject highband_obj = new JSONObject();
            int lnbtype = mDataMananer.getIntParameters(DataMananer.KEY_LNB_TYPE);
            int lowlnb = 0;
            int highlnb = 0;
            int lowMin = 0;
            int lowMax = 11750;
            int highMin = 0;
            int highMax = 11750;
            switch (lnbtype) {
                case 0:
                    lowlnb = 5150;
                    break;
                case 1:
                    lowlnb = 9750;
                    highlnb = 10600;
                    break;
                case 2:
                    String customlnb = mDataMananer.getStringParameters(DataMananer.KEY_LNB_CUSTOM);
                    if (TextUtils.isEmpty(customlnb)) {
                        Log.d(TAG, "customlnb null!");
                        Toast.makeText(this, R.string.dialog_parameter_set_lnb, Toast.LENGTH_SHORT).show();
                        return null;
                    }
                    String[] customlnbvalue = null;
                    if (customlnb != null) {
                        customlnbvalue = customlnb.split(",");
                    }
                    if (customlnbvalue != null && customlnbvalue.length == 1) {
                        lowlnb = Integer.valueOf(customlnbvalue[0]);
                        lowMin = mDataMananer.getIntParameters(DataMananer.KEY_LNB_CUSTOM_LOW_MIN);
                        lowMax = mDataMananer.getIntParameters(DataMananer.KEY_LNB_CUSTOM_LOW_MAX);
                    } else if (customlnbvalue != null && customlnbvalue.length == 2) {
                        lowlnb = Integer.valueOf(customlnbvalue[0]);
                        highlnb = Integer.valueOf(customlnbvalue[1]);
                        lowMin = mDataMananer.getIntParameters(DataMananer.KEY_LNB_CUSTOM_LOW_MIN);
                        lowMax = mDataMananer.getIntParameters(DataMananer.KEY_LNB_CUSTOM_LOW_MAX);
                        highMin = mDataMananer.getIntParameters(DataMananer.KEY_LNB_CUSTOM_HIGH_MIN);
                        highMax = mDataMananer.getIntParameters(DataMananer.KEY_LNB_CUSTOM_HIGH_MAX);
                    } else {
                        Log.d(TAG, "null lnb customized data!");
                        Toast.makeText(this, R.string.dialog_parameter_set_lnb, Toast.LENGTH_SHORT).show();
                        return null;
                    }
                    break;
            }
            lowband_obj.put("min_freq", lowMin);
            lowband_obj.put("max_freq", lowMax);
            lowband_obj.put("local_oscillator_frequency", lowlnb);
            lowband_obj.put("lnb_voltage", DataMananer.DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER_LIST[mDataMananer.getIntParameters(DataMananer.KEY_LNB_POWER)]);
            lowband_obj.put("tone_22k", mDataMananer.getIntParameters(DataMananer.KEY_22_KHZ) == 1);
            highband_obj.put("min_freq", highMin);
            highband_obj.put("max_freq", highMax);
            highband_obj.put("local_oscillator_frequency", highlnb);
            highband_obj.put("lnb_voltage", DataMananer.DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER_LIST[mDataMananer.getIntParameters(DataMananer.KEY_LNB_POWER)]);
            highband_obj.put("tone_22k", mDataMananer.getIntParameters(DataMananer.KEY_22_KHZ) == 1);

            lnbobj.put("low_band", lowband_obj);
            if (highlnb > 0) {
                lnbobj.put("high_band", highband_obj);
            }
            obj.put("lnb", lnbobj);
        } catch (Exception e) {
            obj = null;
            Toast.makeText(this, R.string.dialog_parameter_set_lnb, Toast.LENGTH_SHORT).show();
            Log.d(TAG, "initLbnData Exception " + e.getMessage());
            e.printStackTrace();
        }
        return obj;
    }

     private void testAddSatallite() {
        try {
            JSONArray args = new JSONArray();
            args.put("test1");
            args.put(true);
            args.put(1200);
            Log.d(TAG, "addSatallite->" + args.toString());
            DtvkitGlueClient.getInstance().request("Dvbs.addSatellite", args);
            JSONArray args1 = new JSONArray();
            JSONObject resultObj = DtvkitGlueClient.getInstance().request("Dvbs.getSatellites", args1);
            if (resultObj != null) {
                Log.d(TAG, "addSatallite resultObj:" + resultObj.toString());
            } else {
                Log.d(TAG, "addSatallite then get null");
            }
        } catch (Exception e) {
            Log.d(TAG, "addSatallite Exception " + e.getMessage() + ", trace=" + e.getStackTrace());
            e.printStackTrace();
        }
    }

    private void testAddTransponder() {
        try {
            JSONArray args = new JSONArray();
            args.put("test1");
            args.put(498);
            args.put("H");
            args.put(6875);
            Log.d(TAG, "addTransponder->" + args.toString());
            DtvkitGlueClient.getInstance().request("Dvbs.addTransponder", args);
            JSONArray args1 = new JSONArray();
            JSONObject resultObj = DtvkitGlueClient.getInstance().request("Dvbs.getSatellites", args1);
            if (resultObj != null) {
                Log.d(TAG, "addTransponder resultObj:" + resultObj.toString());
            } else {
                Log.d(TAG, "addTransponder then get null");
            }
        } catch (Exception e) {
            Log.d(TAG, "addTransponder Exception " + e.getMessage() + ", trace=" + e.getStackTrace());
            e.printStackTrace();
        }
    }

    private void startSearch() {
        setSearchStatus("Searching");
        getProgressBar().setIndeterminate(false);
        startMonitoringSearch();

        /*CheckBox network = (CheckBox)findViewById(R.id.network);
        EditText frequency = (EditText)findViewById(R.id.frequency);
        Spinner satellite = (Spinner)findViewById(R.id.satellite);
        Spinner polarity = (Spinner)findViewById(R.id.polarity);
        EditText symbolrate = (EditText)findViewById(R.id.symbolrate);
        Spinner fec = (Spinner)findViewById(R.id.fec);
        CheckBox dvbs2 = (CheckBox)findViewById(R.id.dvbs2);
        Spinner modulation = (Spinner)findViewById(R.id.modulation);*/

        try {
            /*JSONArray args = new JSONArray();
            args.put(network.isChecked());
            args.put(Integer.parseInt(frequency.getText().toString()));
            args.put(satellite.getSelectedItem().toString());
            args.put(polarity.getSelectedItem().toString());
            args.put(Integer.parseInt(symbolrate.getText().toString()));
            args.put(fec.getSelectedItem().toString());
            args.put(dvbs2.isChecked());
            args.put(modulation.getSelectedItem().toString());

            Log.i(TAG, args.toString());

            DtvkitGlueClient.getInstance().request("Dvbs.startManualSearch", args);*/
            JSONArray args = initSearchParameter();
            if (args != null) {
                Log.i(TAG, "search parameter:" + args.toString());
                //DtvkitGlueClient.getInstance().request("Dvbs.startSearch", args);
                //prevent ui not fresh on time
                doSearchByThread(args);
            } else {
                stopMonitoringSearch();
                setSearchStatus(getString(R.string.invalid_parameter));
                enableSearchButton(true);
                stopSearch(true);
                return;
            }
        } catch (Exception e) {
            stopMonitoringSearch();
            setSearchStatus(e.getMessage());
            stopSearch(true);
        }
    }

    private void stopSearch(boolean sync) {
        mStartSearch = false;
        enableSearchButton(true);
        enableStopButton(false);
        setSearchStatus("Finishing search");
        getProgressBar().setIndeterminate(sync);
        stopMonitoringSearch();
        try {
            JSONArray args = new JSONArray();
            args.put(true); // Commit
            DtvkitGlueClient.getInstance().request("Dvbs.finishSearch", args);
        } catch (Exception e) {
            setSearchStatus("Failed to finish search:" + e.getMessage());
            return;
        }
        //setSearchStatus(getString(R.string.strSearchNotStarted));
        if (!sync) {
            return;
        }
        setSearchStatus("Updating guide");
        startMonitoringSync();
        // By default, gets all channels and 1 hour of programs (DEFAULT_IMMEDIATE_EPG_DURATION_MILLIS)
        EpgSyncJobService.cancelAllSyncRequests(this);
        String inputId = this.getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        Log.i(TAG, String.format("inputId: %s", inputId));
        EpgSyncJobService.requestImmediateSync(this, inputId, true, new ComponentName(this, DtvkitEpgSync.class)); // 12 hours
    }

    private void doSearchByThread(final JSONArray args) {
        new Thread(new Runnable() {
            public void run() {
                try {
                    mSearchDvbsType = DtvkitDvbScanSelect.SEARCH_TYPE_DVBS;
                    DtvkitGlueClient.getInstance().request("Dvbs.startSearch", args);
                    mStartSearch = true;
                    mFoundServiceNumber = 0;
                    mServiceList = null;
                } catch (Exception e) {
                    doStopByUiThread(e);
                }
                enableSearchButton(true);
            }
        }).start();
    }

    private void doStopByUiThread(final Exception e) {
        runOnUiThread(new Runnable() {
            public void run() {
                stopMonitoringSearch();
                setSearchStatus(e.getMessage());
            }
        });
    }

    private void enableSetupButton(boolean enable) {
        runOnUiThread(new Runnable() {
            public void run() {
                Button setup = (Button)findViewById(R.id.setup);
                setup.setEnabled(enable);
            }
        });
    }

    private void enableSearchButton(boolean enable) {
        runOnUiThread(new Runnable() {
            public void run() {
                Button search = (Button)findViewById(R.id.startsearch);
                search.setEnabled(enable);
            }
        });
    }

    private void enableStopButton(boolean enable) {
        runOnUiThread(new Runnable() {
            public void run() {
                Button stop = (Button)findViewById(R.id.stopsearch);
                stop.setEnabled(enable);
            }
        });
    }

    private void onSearchFinished() {
        mStartSearch = false;
        enableSearchButton(false);
        enableStopButton(false);
        enableSetupButton(false);
        setSearchStatus("Finishing search");
        getProgressBar().setIndeterminate(true);
        stopMonitoringSearch();
        try {
            JSONArray args = new JSONArray();
            args.put(true); // Commit
            DtvkitGlueClient.getInstance().request("Dvbs.finishSearch", args);
        } catch (Exception e) {
            stopMonitoringSearch();
            setSearchStatus(e.getMessage());
            return;
        }
        //update search results as After the search is finished, the lcn will be reordered
        mServiceList = getServiceList();
        mFoundServiceNumber = getFoundServiceNumber();
        if (mFoundServiceNumber == 0 && mServiceList != null && mServiceList.length() > 0) {
            Log.d(TAG, "mFoundServiceNumber erro use mServiceList length = " + mServiceList.length());
            mFoundServiceNumber = mServiceList.length();
        }

        setSearchStatus("Updating guide");
        startMonitoringSync();
        // By default, gets all channels and 1 hour of programs (DEFAULT_IMMEDIATE_EPG_DURATION_MILLIS)
        EpgSyncJobService.cancelAllSyncRequests(this);
        String inputId = this.getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        Log.i(TAG, String.format("inputId: %s", inputId));
        //EpgSyncJobService.requestImmediateSync(this, inputId, true, new ComponentName(this, DtvkitEpgSync.class)); // 12 hours
        EpgSyncJobService.requestImmediateSyncSearchedChannel(this, inputId, (mFoundServiceNumber > 0),new ComponentName(this, DtvkitEpgSync.class));
    }

    private void startMonitoringSearch() {
        DtvkitGlueClient.getInstance().registerSignalHandler(mHandler);
    }

    private void stopMonitoringSearch() {
        DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
    }

    private void startMonitoringSync() {
        mStartSync = true;
        LocalBroadcastManager.getInstance(this).registerReceiver(mReceiver,
                new IntentFilter(EpgSyncJobService.ACTION_SYNC_STATUS_CHANGED));
    }

    private void stopMonitoringSync() {
        mStartSync = false;
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mReceiver);
    }

    private void setSearchStatus(String status) {
        Log.i(TAG, String.format("Search status \"%s\"", status));
        final TextView text = (TextView) findViewById(R.id.searchstatus);
        text.setText(status);
    }

    private ProgressBar getProgressBar() {
        return (ProgressBar) findViewById(R.id.searchprogress);
    }

    private int getFoundServiceNumber() {
        int found = 0;
        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getNumberOfServices", new JSONArray());
            found = obj.getInt("data");
            Log.i(TAG, "getFoundServiceNumber found = " + found);
        } catch (Exception ignore) {
            Log.e(TAG, "getFoundServiceNumber Exception = " + ignore.getMessage());
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
            Log.e(TAG, "getSearchProcess Exception = " + ignore.getMessage());
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
                //Log.i(TAG, "getServiceList service = " + service.toString());
            }

        } catch (Exception e) {
            Log.e(TAG, "getServiceList Exception = " + e.getMessage());
        }
        return result;
    }
}
