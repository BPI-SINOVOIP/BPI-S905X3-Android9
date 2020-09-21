/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import com.droidlogic.tvinput.R;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TextView;
import android.view.ViewGroup;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlDataManager;

public class ManualScanEdit implements OnFocusChangeListener , OnClickListener, OnCheckedChangeListener{
    public static final String TAG = "ScanEdit";
    public static final int SCAN_ONLY_DTV = 0;
    public static final int SCAN_ONLY_ATV = 1;
    public static final int SCAN_ATV_DTV = 2;
    public static final int SCAN_FAULT = -1;
    public static final int CABLE_MODE_STANDARD = 1;
    public static final int CABLE_MODE_LRC = 2;
    public static final int CABLE_MODE_HRC =3;
    private ViewGroup mParentView;
    private TextView dtvText ;
    private TextView atvText;
    private Switch dtvSwitch;
    private Switch atvSwitch;
    //private RadioButton standardRb;
   // private RadioButton lrcRb;
   // private RadioButton hrcRb;
    private Context mContext;
    private RadioGroup dtv_radio_group;
    private String mInputId;
    private OptionEditText mOptionEditText;
    private TvControlDataManager mTvControlDataManager = null;

    public ManualScanEdit (Context context,ViewGroup viewgroup){
        Log.d(TAG," ScanEdit ");
        mContext = context;
        mParentView = viewgroup;
        initScanView(mParentView);
        Intent mIntent = new Intent();
        mInputId = mIntent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
    }

    private void initScanView(ViewGroup mParent) {
        Log.d(TAG," initScanView ");
        dtvText = (TextView) mParent.findViewById(R.id.tv_dtv_manual);
        atvText = (TextView) mParent.findViewById(R.id.tv_atv_manual);

        dtvSwitch = (Switch) mParent.findViewById(R.id.sw_dtv_manual);
        dtvSwitch.setOnFocusChangeListener(this);
        dtvSwitch.setOnCheckedChangeListener(this);

        atvSwitch = (Switch) mParent.findViewById(R.id.sw_atv_manual);
        atvSwitch.setOnFocusChangeListener(this);
        atvSwitch.setOnCheckedChangeListener(this);

       // dtv_radio_group = (RadioGroup) mParent.findViewById(R.id.dtv_radio_group_manual);

        //standardRb = (RadioButton) mParent.findViewById(R.id.rb_standard_manual);
        //standardRb.setOnFocusChangeListener(this);

        //lrcRb = (RadioButton) mParent.findViewById(R.id.rb_lrc_manual);
        //lrcRb.setOnFocusChangeListener(this);

        //hrcRb = (RadioButton) mParent.findViewById(R.id.rb_hrc_manual);
        //hrcRb.setOnFocusChangeListener(this);
        mOptionEditText =(OptionEditText) mParent.findViewById(R.id.manual_search_dtv_channel_manual);
        initViewNextFocus();
    }

    private void initViewNextFocus() {
        Log.d(TAG," initViewNextFocus ");
        dtvSwitch.setNextFocusUpId(R.id.sw_dtv_manual);
        atvSwitch.setNextFocusDownId(R.id.manual_search_dtv_channel_manual);
        dtvSwitch.setNextFocusLeftId(R.id.content_list);
        dtvSwitch.setNextFocusDownId(R.id.manual_search_dtv_channel_manual);
        atvSwitch.setNextFocusUpId(R.id.sw_atv_manual);
        //standardRb.setNextFocusLeftId(R.id.content_list);
        //standardRb.setNextFocusRightId(R.id.rb_standard_manual);
        //lrcRb.setNextFocusLeftId(R.id.content_list);
        //lrcRb.setNextFocusRightId(R.id.rb_lrc_manual);
        //hrcRb.setNextFocusLeftId(R.id.content_list);
        //hrcRb.setNextFocusRightId(R.id.rb_hrc_manual);
        mOptionEditText.setNextFocusUpId(R.id.sw_dtv_manual);
        /*if (dtvSwitch.isChecked() && getTvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
            dtv_radio_group.setVisibility(View.VISIBLE);
            atvSwitch.setNextFocusDownId(R.id.rb_standard_manual);
            dtvSwitch.setNextFocusDownId(R.id.rb_standard_manual);
            mOptionEditText.setNextFocusUpId(R.id.rb_lrc_manual);
        }*/
    }

    private void initViewBackgroundColor() {
        Log.d(TAG," initViewBackgroundColor ");
        dtvText.setBackgroundColor(android.R.color.transparent);
        dtvText.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
        atvText.setBackgroundColor(android.R.color.transparent);
        atvText.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
        //standardRb.setBackgroundColor(android.R.color.transparent);
        //standardRb.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
        //lrcRb.setBackgroundColor(android.R.color.transparent);
        //lrcRb.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
        //hrcRb.setBackgroundColor(android.R.color.transparent);
        //hrcRb.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
    }
    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        Log.d(TAG," onFocusChange ");
        if (v instanceof TextView) {
            initViewBackgroundColor();
            switch (v.getId()) {
            case R.id.sw_dtv_manual:
                if (!hasFocus) {
                    dtvText.setBackgroundColor(android.R.color.transparent);
                    dtvText.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
                }else {
                    dtvText.setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
                    dtvText.setBackgroundResource(R.color.selected);
                }
                break;
            case R.id.sw_atv_manual:
                if (!hasFocus) {
                    atvText.setBackgroundColor(android.R.color.transparent);
                    atvText.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
                }else {
                    atvText.setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
                    atvText.setBackgroundResource(R.color.selected);
                }
                break;
           /* case R.id.rb_standard_manual:
                if (!hasFocus) {
                    standardRb.setBackgroundColor(android.R.color.transparent);
                    standardRb.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
                }else {
                    standardRb.setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
                    standardRb.setBackgroundResource(R.color.selected);
                }
                break;
            case R.id.rb_lrc_manual:
                if (!hasFocus) {
                    lrcRb.setBackgroundColor(android.R.color.transparent);
                    lrcRb.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
                }else {
                    lrcRb.setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
                    lrcRb.setBackgroundResource(R.color.selected);
                }
                break;
            case R.id.rb_hrc_manual:
                if (!hasFocus) {
                    hrcRb.setBackgroundColor(android.R.color.transparent);
                    hrcRb.setTextColor(mContext.getResources().getColor(R.color.color_text_item));
                }else {
                    hrcRb.setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
                    hrcRb.setBackgroundResource(R.color.selected);
                }
                break;*/
            }
        }
    }

    @Override
    public void onClick(View v) {
        Log.d(TAG," onClick ");
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        Log.d(TAG," onCheckedChanged ");
        switch (buttonView.getId()) {
        case R.id.sw_atv_manual:
            if (!isChecked) {
                if (!dtvSwitch.isChecked()) {
                     dtvSwitch.setChecked(true);
                }
            }
            break;
        case R.id.sw_dtv_manual:
            if (isChecked) {
                if (getTvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    //dtv_radio_group.setVisibility(View.VISIBLE);
                    atvSwitch.setNextFocusDownId(R.id.manual_search_dtv_channel_manual);
                    dtvSwitch.setNextFocusDownId(R.id.manual_search_dtv_channel_manual);
                    mOptionEditText.setNextFocusUpId(R.id.sw_dtv_manual);
                }
            } else {
                if (!atvSwitch.isChecked()) {
                    atvSwitch.setChecked(true);
                }
                //dtv_radio_group.setVisibility(View.GONE);
                atvSwitch.setNextFocusDownId(R.id.manual_search_dtv_channel_manual);
                dtvSwitch.setNextFocusDownId(R.id.manual_search_dtv_channel_manual);
                mOptionEditText.setNextFocusUpId(R.id.sw_dtv_manual);
            }
            break;
        }
    }

    public int checkAutoScanMode() {
        Log.d(TAG," checkAutoScanMode ");
        if (dtvSwitch.isChecked() && !atvSwitch.isChecked()) {
            return SCAN_ONLY_DTV;
        } else if (!dtvSwitch.isChecked() && atvSwitch.isChecked()){
            return SCAN_ONLY_ATV;
        }else if (dtvSwitch.isChecked() && atvSwitch.isChecked()){
            return SCAN_ATV_DTV;
        }else {
            return SCAN_FAULT;
        }
    }

    public int checkCableMode() {
        Log.d(TAG," checkCableMode ");
        /*if (standardRb.isChecked() && !lrcRb.isChecked() ) {
            return CABLE_MODE_STANDARD;
        }else if(!standardRb.isChecked() && lrcRb.isChecked() ) {
            return CABLE_MODE_LRC;
        }*/
        return 0;
    }

    private String getTvType() {
        int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
        String type = mTvControlDataManager.getString(mContext.getContentResolver(),
            DroidLogicTvUtils.TV_KEY_DTV_TYPE);
        if (type != null) {
            return type;
        } else {
            if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV ) {
                return TvContract.Channels.TYPE_ATSC_T;
            }else {
                return TvContract.Channels.TYPE_DTMB;
            }
        }
    }
}

