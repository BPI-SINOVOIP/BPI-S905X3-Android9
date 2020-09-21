/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC OutputUiManager
 */

package com.droidlogic.tv.settings.display.outputmode;

import com.droidlogic.tv.settings.R;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.DolbyVisionSettingManager;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserHandle ;

public class OutputUiManager {
    private static final String TAG = "OutputUiManager";
    private static boolean DEBUG = false;

    public static final String CVBS_MODE = "cvbs";
    public static final String HDMI_MODE = "hdmi";

    private static final String[] HDMI_LIST = {
        "2160p60hz",
        "2160p50hz",
        "2160p30hz",
        "2160p25hz",
        "2160p24hz",
        "smpte24hz",
        "1080p60hz",
        "1080p50hz",
        "1080p24hz",
        "720p60hz",
        "720p50hz",
        "1080i60hz",
        "1080i50hz",
        "576p50hz",
        "480p60hz",
        "576i50hz",
        "480i60hz",
    };
    private static final String[] HDMI_TITLE = {
        "4k2k-60hz",
        "4k2k-50hz",
        "4k2k-30hz",
        "4k2k-25hz",
        "4k2k-24hz",
        "4k2k-smpte",
        "1080p-60hz",
        "1080p-50hz",
        "1080p-24hz",
        "720p-60hz",
        "720p-50hz",
        "1080i-60hz",
        "1080i-50hz",
        "576p-50hz",
        "480p-60hz",
        "576i-50hz",
        "480i-60hz"
    };

    private static final String[] HDMI_COLOR_LIST = {
        "444,12bit",
        "444,10bit",
        "444,8bit",
        "422,12bit",
        "422,10bit",
        "422,8bit",
        "420,12bit",
        "420,10bit",
        "420,8bit",
        "rgb,12bit",
        "rgb,10bit",
        "rgb,8bit"
    };
    private static final String[] HDMI_COLOR_TITLE = {
        "YCbCr444 12bit",
        "YCbCr444 10bit",
        "YCbCr444 8bit",
        "YCbCr422 12bit",
        "YCbCr422 10bit",
        "YCbCr422 8bit",
        "YCbCr420 12bit",
        "YCbCr420 10bit",
        "YCbCr420 8bit",
        "RGB 12bit",
        "RGB 10bit",
        "RGB 8bit"
    };

    public static final String[] COLOR_SPACE_LIST = {
        "444",
        "422",
        "420",
        "rgb",
    };

    public static final String[] COLOR_SPACE_TITLE = {
        "YCbCr444",
        "YCbCr422",
        "YCbCr420",
        "RGB",
    };

    public static final String[] COLOR_DEPTH_LIST = {
        "16bit",
        "12bit",
        "10bit",
        "8bit",
    };

    private static final String[] CVBS_MODE_VALUE_LIST = {
        "480cvbs",
        "576cvbs"
    };
    private static final String[] CVBS_MODE_TITLE_LIST = {
        "480 CVBS",
        "576 CVBS"
    };

    private static final String[] DOLBY_VISION_TYPE = {
         "DV_RGB_444_8BIT",
//         "DV_YCbCr_422_12BIT",
         "LL_YCbCr_422_12BIT",
         "LL_RGB_444_12BIT",
         "LL_RGB_444_10BIT"
    };

    private static final int DV_LL_RGB            = 3;
    private static final int DV_LL_YUV            = 2;
    private static final int DV_ENABLE            = 1;
    private static final int DV_DISABLE           = 0;

    private static final int DEFAULT_HDMI_MODE = 0;
    private static final int DEFAULT_CVBS_MODE = 1;
    private static String[] mHdmiValueList;
    private static String[] mHdmiTitleList;

    private static String[] mHdmiColorValueList;
    private static String[] mHdmiColorTitleList;

    private ArrayList<String> mTitleList = new ArrayList<String>();
    private ArrayList<String> mValueList = new ArrayList<String>();
    private ArrayList<String> mSupportList = new ArrayList<String>();

    private ArrayList<String> mColorTitleList = new ArrayList<String>();
    private ArrayList<String> mColorValueList = new ArrayList<String>();

    private OutputModeManager mOutputModeManager;
    private DolbyVisionSettingManager mDolbyVisionSettingManager;
    private Context mContext;

    private static String mUiMode;
    private static String tvSupportDolbyVisionMode;
    private static String tvSupportDolbyVisionType;

    public OutputUiManager(Context context){
        mContext = context;
        mOutputModeManager = new OutputModeManager(mContext);
        mDolbyVisionSettingManager = new DolbyVisionSettingManager(mContext);

        mUiMode = getUiMode();
        initModeValues(mUiMode);
        initColorValues(mUiMode);
    }

    public String getUiMode(){
        String currentMode = mOutputModeManager.getCurrentOutputMode();
        Log.d(TAG,"getUiMode currentMode = " + currentMode);
        if (currentMode.contains(CVBS_MODE)) {
            mUiMode = CVBS_MODE;
        } else {
            mUiMode = HDMI_MODE;
        }
        return mUiMode;
    }

    public void updateUiMode(){
        mUiMode = getUiMode();
        initModeValues(mUiMode);
        initColorValues(mUiMode);
    }

    public boolean isHdmiMode() {
        if (mUiMode.contains("cvbs"))
            return false;
        return true;
    }

    public String getCurrentMode(){
         return mOutputModeManager.getCurrentOutputMode();
    }

    public String getCurrentColorAttribute(){
         return mOutputModeManager.getCurrentColorAttribute();
    }

    public String getCurrentColorSpaceAttr() {
        for (int i = 0; i < COLOR_SPACE_LIST.length; i++) {
            if (getCurrentColorAttribute().contains(COLOR_SPACE_LIST[i])) {
                return COLOR_SPACE_LIST[i];
            }
        }
        return "default";
    }

    public void setHdrStrategy(String type){
        mOutputModeManager.setHdrStrategy(type);
    }

    public String getHdrStrategy() {
        return mOutputModeManager.getHdrStrategy();
    }
    public String getCurrentColorSpaceTitle() {
        for (int i = 0; i < COLOR_SPACE_LIST.length; i++) {
            if (getCurrentColorAttribute().contains(COLOR_SPACE_LIST[i])) {
                return COLOR_SPACE_TITLE[i];
            }
        }
        return "default";
    }

    public String getCurrentColorDepthAttr() {
        for (int i = 0; i < COLOR_DEPTH_LIST.length; i++) {
            if (getCurrentColorAttribute().contains(COLOR_DEPTH_LIST[i])) {
                return COLOR_DEPTH_LIST[i];
            }
        }
        return "default";
    }

    private void initColorValues(String mode){
        filterColorAttribute();
        mColorTitleList.clear();
        mColorValueList.clear();

        if (mode.equalsIgnoreCase(HDMI_MODE)) {
            for (int i=0 ; i< mHdmiColorValueList.length; i++) {
                if (mHdmiColorTitleList[i] != null && mHdmiColorTitleList[i].length() != 0) {
                    mColorTitleList.add(mHdmiColorTitleList[i]);
                    mColorValueList.add(mHdmiColorValueList[i]);
                }
            }
        }
    }

    public void changeColorAttribte(final String colorValue) {
        mOutputModeManager.setDeepColorAttribute(colorValue);
    }

    public ArrayList<String> getColorTitleList(){
        return mColorTitleList;
    }

    public ArrayList<String> getColorValueList(){
        return mColorValueList;
    }

    public void  filterColorAttribute() {
        List<String> listValue = new ArrayList<String>();
        List<String> listTitle = new ArrayList<String>();

        mHdmiColorValueList = HDMI_COLOR_LIST;
        mHdmiColorTitleList = HDMI_COLOR_TITLE;

        for (int i = 0; i < mHdmiColorValueList.length; i++) {
            if (mHdmiColorValueList[i] != null) {
                listValue.add(mHdmiColorValueList[i]);
                listTitle.add(mHdmiColorTitleList[i]);
            }
        }

        String strColorlist = mOutputModeManager.getHdmiColorSupportList();
        if (strColorlist != null && strColorlist.length() != 0 && !strColorlist.contains("null")) {
            List<String> listHdmiMode = new ArrayList<String>();
            List<String> listHdmiTitle = new ArrayList<String>();
            for (int i = 0; i < listValue.size(); i++) {
                if (strColorlist.contains(listValue.get(i))) {
                    listHdmiMode.add(listValue.get(i));
                    listHdmiTitle.add(listTitle.get(i));
                }

            }
            mHdmiColorValueList = listHdmiMode.toArray(new String[listValue.size()]);
            mHdmiColorTitleList = listHdmiTitle.toArray(new String[listTitle.size()]);
        } else {
            mHdmiColorValueList = new String[]{""};
            mHdmiColorTitleList = new String[]{"No data!"};
        }
    }

    public boolean isModeSupportColor(final String curMode, final String curValue){
        return mOutputModeManager.isModeSupportColor(curMode, curValue);
    }

    public int getCurrentModeIndex(){
         String currentMode = mOutputModeManager.getCurrentOutputMode();
         for (int i=0 ; i < mValueList.size();i++) {
             if (currentMode.equals(mValueList.get(i))) {
                return i ;
             }
         }
         if (mUiMode.equals(HDMI_MODE)) {
            return DEFAULT_HDMI_MODE;
         }else{
            return DEFAULT_CVBS_MODE;
         }
    }

    private void initModeValues(String mode){
        filterOutputMode();
        mTitleList.clear();
        mValueList.clear();

        if (mode.equalsIgnoreCase(HDMI_MODE)) {
            for (int i=0 ; i< mHdmiValueList.length; i++) {
                if (mHdmiTitleList[i] != null && mHdmiTitleList[i].length() != 0) {
                    mTitleList.add(mHdmiTitleList[i]);
                    mValueList.add(mHdmiValueList[i]);
                }
            }
        }else if (mode.equalsIgnoreCase(CVBS_MODE)) {
            for (int i = 0 ; i< CVBS_MODE_VALUE_LIST.length; i++) {
                mTitleList.add(CVBS_MODE_VALUE_LIST[i]);
            }
            for (int i=0 ; i < CVBS_MODE_VALUE_LIST.length ; i++) {
                mValueList.add(CVBS_MODE_VALUE_LIST[i]);
            }
        }
    }

    public void change2NewMode(final String mode) {
        mOutputModeManager.setBestMode(mode);
    }

    public void change2BestMode() {
        mOutputModeManager.setBestMode(null);
    }
    public boolean isBestOutputmode(){
        return mOutputModeManager.isBestOutputmode();
    }

    public void setBestDolbyVision(boolean enable) {
        mOutputModeManager.setBestDolbyVision(enable);
    }
    public boolean isBestDolbyVsion(){
        return mOutputModeManager.isBestDolbyVsion();
    }
    public void change2DeepColorMode() {
        mOutputModeManager.setDeepColorMode();
    }

    public boolean isDeepColor(){
        return mOutputModeManager.isDeepColor();
    }

    public ArrayList<String> getOutputmodeTitleList(){
        return mTitleList;
    }

    public ArrayList<String> getOutputmodeValueList(){
        return mValueList;
    }

    public void  filterOutputMode() {
        List<String> listValue = new ArrayList<String>();
        List<String> listTitle = new ArrayList<String>();

        mHdmiValueList = HDMI_LIST;
        mHdmiTitleList = HDMI_TITLE;

        for (int i = 0; i < mHdmiValueList.length; i++) {
            if (mHdmiValueList[i] != null) {
                listValue.add(mHdmiValueList[i]);
                listTitle.add(mHdmiTitleList[i]);
            }
        }

        String strFilterMode = mContext.getResources().getString(R.string.display_filter_outputmode);
        if (strFilterMode != null && strFilterMode.length() != 0) {
            String[] array_filter_mode = strFilterMode.split(",");
            for (int i = 0; i < array_filter_mode.length; i++) {
                for (int j = 0; j < listValue.size(); j++) {
                    if ((listValue.get(j).toString()).equals(array_filter_mode[i])) {
                        listValue.remove(j);
                        listTitle.remove(j);
                    }
                }
            }
        }

        String strEdid = mOutputModeManager.getHdmiSupportList();
        if (strEdid != null && strEdid.length() != 0 && !strEdid.contains("null")) {
            List<String> listHdmiMode = new ArrayList<String>();
            List<String> listHdmiTitle = new ArrayList<String>();
            for (int i = 0; i < listValue.size(); i++) {
                if (strEdid.contains(listValue.get(i))) {
                    listHdmiMode.add(listValue.get(i));
                    listHdmiTitle.add(listTitle.get(i));
                }

            }

            List<String> listHdmiMode_tmp = new ArrayList<String>();
            List<String> listHdmiTitle_tmp = new ArrayList<String>();
            if (isDolbyVisionEnable() && isTvSupportDolbyVision()) {
                Log.d(TAG,"zzy filter isDolbyVisionEnable");
                for (int i = 0; i < listHdmiMode.size(); i++) {
                    if (resolveResolutionValue(listHdmiMode.get(i))
                            > resolveResolutionValue(tvSupportDolbyVisionMode)) {
                        Log.e(TAG, "This TV not Support Dolby Vision in " + listHdmiMode.get(i));
                    } else {
                        if (isDolbyVisionEnable() && isTvSupportDolbyVision()) {
                            if (listHdmiMode.get(i).contains("smpte") || listHdmiMode.get(i).contains("i")) {
                                continue;
                            }
                            int type = mDolbyVisionSettingManager.getDolbyVisionType();
                            switch (type) {
                                case DV_ENABLE:
                                    if (isModeSupportColor(listHdmiMode.get(i), "444,8bit")) {
                                        listHdmiMode_tmp.add(listHdmiMode.get(i));
                                        listHdmiTitle_tmp.add(listHdmiTitle.get(i));
                                    }
                                    break;
                                case DV_LL_YUV:
                                    if (isModeSupportColor(listHdmiMode.get(i), "422,12bit")
                                            || isModeSupportColor(listHdmiMode.get(i), "422,10bit")) {
                                        listHdmiMode_tmp.add(listHdmiMode.get(i));
                                        listHdmiTitle_tmp.add(listHdmiTitle.get(i));
                                    }
                                    break;
                                case DV_LL_RGB:
                                    if (resolveResolutionValue(listHdmiMode.get(i))
                                            > resolveResolutionValue("1080p60hz")) {
                                        continue;
                                    }

                                    if (isModeSupportColor(listHdmiMode.get(i), "444,12bit")
                                            || isModeSupportColor(listHdmiMode.get(i), "444,10bit")) {
                                        listHdmiMode_tmp.add(listHdmiMode.get(i));
                                        listHdmiTitle_tmp.add(listHdmiTitle.get(i));
                                    }
                                    break;
                            }
                        } else {
                            listHdmiMode_tmp.add(listHdmiMode.get(i));
                            listHdmiTitle_tmp.add(listHdmiTitle.get(i));
                        }
                    }
                }
                mHdmiValueList = listHdmiMode_tmp.toArray(new String[listValue.size()]);
                mHdmiTitleList = listHdmiTitle_tmp.toArray(new String[listTitle.size()]);
            } else {
                mHdmiValueList = listHdmiMode.toArray(new String[listValue.size()]);
                mHdmiTitleList = listHdmiTitle.toArray(new String[listTitle.size()]);
            }
        } else {
            mHdmiValueList = listValue.toArray(new String[listValue.size()]);
            mHdmiTitleList = listTitle.toArray(new String[listTitle.size()]);
        }
    }

    public boolean isTvSupportDolbyVision() {
        String dv_cap = mDolbyVisionSettingManager.isTvSupportDolbyVision();
        tvSupportDolbyVisionType = null;
        if (!dv_cap.equals("")) {
            for (int i = 0;i < HDMI_LIST.length; i++) {
                if (dv_cap.contains(HDMI_LIST[i])) {
                    tvSupportDolbyVisionMode = HDMI_LIST[i];
                    break;
                }
            }
            for (int i = 0; i < DOLBY_VISION_TYPE.length; i++) {
                if (dv_cap.contains(DOLBY_VISION_TYPE[i])) {
                    tvSupportDolbyVisionType += DOLBY_VISION_TYPE[i];
                }
            }
        } else {
            tvSupportDolbyVisionMode = "";
            tvSupportDolbyVisionType = "";
        }

        return tvSupportDolbyVisionMode.equals("") ? false : true;
    }

    public boolean isDolbyVisionEnable() {
        return mDolbyVisionSettingManager.isDolbyVisionEnable();
    }

    public void switchDolbyVisionState() {
        mDolbyVisionSettingManager.setDolbyVisionEnable(isDolbyVisionEnable() ? 0 : 1);
    }

    public long resolveResolutionValue(String mode) {
        return mDolbyVisionSettingManager.resolveResolutionValue(mode);
    }

    public void checkOutputmodeDeepcolor() {
        String mode = getCurrentMode();
        if ((tvSupportDolbyVisionMode != null) && (tvSupportDolbyVisionMode.contains("hz"))
            && (resolveResolutionValue(mode) > resolveResolutionValue(tvSupportDolbyVisionMode))) {
            change2NewMode(tvSupportDolbyVisionMode);
        }

        if ((getCurrentColorAttribute() == null)
                || (!getCurrentColorAttribute().equals("444,8bit"))) {
            changeColorAttribte("444,8bit");
        }
    }
}
