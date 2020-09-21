package com.droidlogic.settings;

import android.util.Log;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Spinner;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.text.TextUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import com.droidlogic.fragment.ParameterMananer;

import org.droidlogic.dtvkit.DtvkitGlueClient;
import org.dtvkit.inputsource.R;

import android.content.ComponentName;
import android.media.tv.TvInputInfo;
import org.dtvkit.companionlibrary.EpgSyncJobService;
import org.dtvkit.inputsource.DtvkitEpgSync;

import com.droidlogic.settings.SysSettingManager;
import com.droidlogic.settings.PropSettingManager;

public class DtvkitDvbSettings extends Activity {

    private static final String TAG = "DtvkitDvbSettings";
    private static final boolean DEBUG = true;

    private DtvkitGlueClient mDtvkitGlueClient = DtvkitGlueClient.getInstance();
    private ParameterMananer mParameterMananer = null;
    private SysSettingManager mSysSettingManager = null;
    private List<String> mStoragePathList = new ArrayList<String>();
    private List<String> mStorageNameList = new ArrayList<String>();
    private Object mStorageLock = new Object();
    private boolean needClearAudioLangSetting = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.lanuage_settings);
        mParameterMananer = new ParameterMananer(this, mDtvkitGlueClient);
        mSysSettingManager = new SysSettingManager(this);
        updateStorageList();
        initLayout(false);
    }

    @Override
    protected void onResume() {
        super.onResume();
        needClearAudioLangSetting = false;
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (needClearAudioLangSetting) {
            mParameterMananer.clearUserAudioSelect();
        }
    }

    private void updatingGuide() {
        EpgSyncJobService.cancelAllSyncRequests(this);
        String inputId = this.getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        Log.i(TAG, String.format("inputId: %s", inputId));
        EpgSyncJobService.requestImmediateSync(this, inputId, true, new ComponentName(this, DtvkitEpgSync.class));
    }

    private void initLayout(boolean update) {
        Spinner country = (Spinner)findViewById(R.id.country_spinner);
        Spinner main_audio = (Spinner)findViewById(R.id.main_audio_spinner);
        Spinner assist_audio = (Spinner)findViewById(R.id.assist_audio_spinner);
        Spinner main_subtitle = (Spinner)findViewById(R.id.main_subtitle_spinner);
        Spinner assist_subtitle = (Spinner)findViewById(R.id.assist_subtitle_spinner);
        Spinner hearing_impaired = (Spinner)findViewById(R.id.hearing_impaired_spinner);
        Spinner pvr_path = (Spinner)findViewById(R.id.storage_select_spinner);
        Button refresh = (Button)findViewById(R.id.storage_refresh);
        CheckBox recordFrequency = (CheckBox)findViewById(R.id.record_full_frequency);
        initSpinnerParameter();
        if (update) {
            Log.d(TAG, "initLayout update");
            return;
        }
        country.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "country onItemSelected position = " + position);
                int saveCountryCode = mParameterMananer.getCurrentCountryCode();
                int selectCountryCode = mParameterMananer.getCountryCodeByIndex(position);
                String previousMainAudioName = mParameterMananer.getCurrentMainAudioLangName();
                String previousAssistAudioName = mParameterMananer.getCurrentSecondAudioLangName();
                if (saveCountryCode == selectCountryCode) {
                    Log.d(TAG, "country onItemSelected same position");
                    return;
                }
                mParameterMananer.setCountryCodeByIndex(position);
                updatingGuide();
                initLayout(true);
                String currentMainAudioName = mParameterMananer.getCurrentMainAudioLangName();
                String currentAssistAudioName = mParameterMananer.getCurrentSecondAudioLangName();
                if (!TextUtils.equals(previousMainAudioName, currentMainAudioName) || !TextUtils.equals(previousAssistAudioName, currentAssistAudioName)) {
                    needClearAudioLangSetting = true;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        main_audio.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "main_audio onItemSelected position = " + position);
                int currentMainAudio = mParameterMananer.getCurrentMainAudioLangId();
                int savedMainAudio = mParameterMananer.getLangIndexCodeByPosition(position);
                if (currentMainAudio == savedMainAudio) {
                    Log.d(TAG, "main_audio onItemSelected same position");
                    return;
                }
                mParameterMananer.setPrimaryAudioLangByPosition(position);
                updatingGuide();
                needClearAudioLangSetting = true;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        assist_audio.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "assist_audio onItemSelected position = " + position);
                int currentAssistAudio = mParameterMananer.getCurrentSecondAudioLangId();
                int savedAssistAudio = mParameterMananer.getSecondLangIndexCodeByPosition(position);
                if (currentAssistAudio == savedAssistAudio) {
                    Log.d(TAG, "assist_audio onItemSelected same position");
                    return;
                }
                mParameterMananer.setSecondaryAudioLangByPosition(position);
                updatingGuide();
                needClearAudioLangSetting = true;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        main_subtitle.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "main_subtitle onItemSelected position = " + position);
                int currentMainSub = mParameterMananer.getCurrentMainSubLangId();
                int savedMainSub = mParameterMananer.getLangIndexCodeByPosition(position);
                if (currentMainSub == savedMainSub) {
                    Log.d(TAG, "main_subtitle onItemSelected same position");
                    return;
                }
                mParameterMananer.setPrimaryTextLangByPosition(position);
                updatingGuide();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        assist_subtitle.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "assist_subtitle onItemSelected position = " + position);
                int currentAssistSub = mParameterMananer.getCurrentSecondSubLangId();
                int savedAssistSub = mParameterMananer.getSecondLangIndexCodeByPosition(position);
                if (currentAssistSub == savedAssistSub) {
                    Log.d(TAG, "assist_subtitle onItemSelected same position");
                    return;
                }
                mParameterMananer.setSecondaryTextLangByPosition(position);
                updatingGuide();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        hearing_impaired.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "hearing_impaired onItemSelected position = " + position);
                int saved = mParameterMananer.getHearingImpairedSwitchStatus() ? 1 : 0;
                if (saved == position) {
                    Log.d(TAG, "hearing_impaired onItemSelected same position");
                    return;
                }
                mParameterMananer.setHearingImpairedSwitchStatus(position == 1);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        pvr_path.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String saved = mParameterMananer.getStringParameters(ParameterMananer.KEY_PVR_RECORD_PATH);
                String current = getCurrentStoragePath(position);
                Log.d(TAG, "pvr_path onItemSelected previous = " + saved + ", new = " + current);
                if (TextUtils.equals(current, saved)) {
                    Log.d(TAG, "pvr_path onItemSelected same path");
                    return;
                }
                mParameterMananer.saveStringParameters(ParameterMananer.KEY_PVR_RECORD_PATH, current);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });
        refresh.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "refresh storage device");
                updateStorageList();
                initLayout(true);
            }
        });
        recordFrequency.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (recordFrequency.isChecked()) {
                    PropSettingManager.setProp(PropSettingManager.PVR_RECORD_MODE, PropSettingManager.PVR_RECORD_MODE_FREQUENCY);
                } else {
                    PropSettingManager.setProp(PropSettingManager.PVR_RECORD_MODE, PropSettingManager.PVR_RECORD_MODE_CHANNEL);
                }
            }
        });
    }

    private void initSpinnerParameter() {
        Spinner country = (Spinner)findViewById(R.id.country_spinner);
        Spinner main_audio = (Spinner)findViewById(R.id.main_audio_spinner);
        Spinner assist_audio = (Spinner)findViewById(R.id.assist_audio_spinner);
        Spinner main_subtitle = (Spinner)findViewById(R.id.main_subtitle_spinner);
        Spinner assist_subtitle = (Spinner)findViewById(R.id.assist_subtitle_spinner);
        Spinner hearing_impaired = (Spinner)findViewById(R.id.hearing_impaired_spinner);
        Spinner pvr_path = (Spinner)findViewById(R.id.storage_select_spinner);
        CheckBox recordFrequency = (CheckBox)findViewById(R.id.record_full_frequency);
        List<String> list = null;
        ArrayAdapter<String> adapter = null;
        int select = 0;
        //add country
        list = mParameterMananer.getCountryDisplayList();
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        country.setAdapter(adapter);
        select = mParameterMananer.getCurrentCountryIndex();
        country.setSelection(select);
        //add main audio
        list = mParameterMananer.getCurrentLangNameList();
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        main_audio.setAdapter(adapter);
        select = mParameterMananer.getCurrentMainAudioLangId();
        main_audio.setSelection(select);
        //add second audio
        list = mParameterMananer.getCurrentSecondLangNameList();
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        assist_audio.setAdapter(adapter);
        select = mParameterMananer.getCurrentSecondAudioLangId();
        assist_audio.setSelection(select);
        //add main subtitle
        list = mParameterMananer.getCurrentLangNameList();
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        main_subtitle.setAdapter(adapter);
        select = mParameterMananer.getCurrentMainSubLangId();
        main_subtitle.setSelection(select);
        //add second subtitle
        list = mParameterMananer.getCurrentSecondLangNameList();
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        assist_subtitle.setAdapter(adapter);
        select = mParameterMananer.getCurrentSecondSubLangId();
        assist_subtitle.setSelection(select);
        //add hearing impaired
        list = getHearingImpairedOption();
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        hearing_impaired.setAdapter(adapter);
        select = mParameterMananer.getHearingImpairedSwitchStatus() ? 1 : 0;
        hearing_impaired.setSelection(select);
        //add pvr path select
        list = mStorageNameList;
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        pvr_path.setAdapter(adapter);
        String devicePath = mParameterMananer.getStringParameters(ParameterMananer.KEY_PVR_RECORD_PATH);
        if (!SysSettingManager.isDeviceExist(devicePath)) {
            select = 0;
            mParameterMananer.saveStringParameters(ParameterMananer.KEY_PVR_RECORD_PATH, SysSettingManager.PVR_DEFAULT_PATH);
        } else {
            select = getCurrentStoragePosition(devicePath);
        }
        pvr_path.setSelection(select);
        String pvrRecordMode = PropSettingManager.getString(PropSettingManager.PVR_RECORD_MODE, PropSettingManager.PVR_RECORD_MODE_CHANNEL);
        recordFrequency.setChecked(PropSettingManager.PVR_RECORD_MODE_FREQUENCY.equals(pvrRecordMode) ? true : false);
    }

    private List<String> getHearingImpairedOption() {
        List<String> result = new ArrayList<String>();
        result.add(getString(R.string.strSettingsHearingImpairedOff));
        result.add(getString(R.string.strSettingsHearingImpairedOn));
        return result;
    }

    private void updateStorageList() {
        synchronized(mStorageLock) {
            mStorageNameList = mSysSettingManager.getStorageDeviceNameList();
            mStoragePathList = mSysSettingManager.getStorageDevicePathList();
        }
    }

    private int getCurrentStoragePosition(String name) {
        int result = 0;
        boolean found = false;
        synchronized(mStorageLock) {
            if (mStoragePathList != null && mStoragePathList.size() > 0) {
                for (int i = 0; i < mStoragePathList.size(); i++) {
                    if (TextUtils.equals(mStoragePathList.get(i), name)) {
                        result = i;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            mParameterMananer.saveStringParameters(ParameterMananer.KEY_PVR_RECORD_PATH, mSysSettingManager.getAppDefaultPath());
        }
        return result;
    }

    private String getCurrentStoragePath(int position) {
        String result = null;
        synchronized(mStorageLock) {
            result = mStoragePathList.get(position);
        }
        return result;
    }
}
