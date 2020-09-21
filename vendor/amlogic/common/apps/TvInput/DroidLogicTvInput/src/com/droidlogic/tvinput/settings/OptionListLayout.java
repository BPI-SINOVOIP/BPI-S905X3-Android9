/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import com.droidlogic.tvinput.R;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvControlManager.UpgradeFBCListener;
import android.os.Build;

public class OptionListLayout implements OnItemClickListener,UpgradeFBCListener {
    private static final String TAG = "OptionListLayout";

    private Context mContext;
    private SettingsManager mSettingsManager;
    private int mTag = -1;
    private View optionView = null;

    private TvControlManager mTvControlManager;
    private String listitem;
    public OptionListLayout (Context context, View view, int tag) {
        mContext = context;
        optionView = view;
        mTag = tag;

        initOptionListView();
    }

    ArrayList<HashMap<String, Object>> optionListData = new ArrayList<HashMap<String, Object>>();
    private void initOptionListView () {
        SimpleAdapter optionAdapter = null;
        TextView title = (TextView)optionView.findViewById(R.id.option_title);
        OptionListView optionListView = (OptionListView)optionView.findViewById(R.id.option_list);

        switch (mTag) {
            case OptionUiManager.OPTION_CHANNEL_INFO:
                title.setText(mContext.getResources().getString(R.string.channel_info));
                optionListData = ((TvSettingsActivity)mContext).getSettingsManager().getChannelInfo();
                optionAdapter = new SimpleAdapter(mContext, optionListData,
                                                  R.layout.layout_option_double_text,
                                                  new String[] {SettingsManager.STRING_NAME, SettingsManager.STRING_STATUS},
                                                  new int[] {R.id.text_name, R.id.text_status});
                break;
            case OptionUiManager.OPTION_DEFAULT_LANGUAGE:
                title.setText(mContext.getResources().getString(R.string.defalut_lan));
                optionListData = ((TvSettingsActivity)mContext).getSettingsManager().getDefaultLanguageList();
                optionAdapter = new SimpleAdapter(mContext, optionListData,
                                                  R.layout.layout_option_single_text,
                                                  new String[] {SettingsManager.STRING_NAME}, new int[] {R.id.text_name});
                break;
            case OptionUiManager.OPTION_FBC_UPGRADE:
                title.setText(mContext.getResources().getString(R.string.fbc_upgrade));
                RecursionAddFile(FolderFilter(), optionListData);
                optionAdapter = new SimpleAdapter(mContext, optionListData,
                                                  R.layout.layout_option_single_text,
                                                  new String[] { SettingsManager.STRING_NAME }, new int[] { R.id.text_name });
                break;
            case OptionUiManager.OPTION_AD_LIST:
                title.setText(mContext.getResources().getString(R.string.ad_list));
                optionListData = ((TvSettingsActivity)mContext).getSettingsManager().getAudioADList();
                optionAdapter = new SimpleAdapter(mContext, optionListData,
                                                  R.layout.layout_option_single_text,
                                                  new String[] {SettingsManager.STRING_NAME}, new int[] {R.id.text_name});
                break;
        }
        if (optionAdapter != null) {
            optionListView.setAdapter(optionAdapter);
            optionListView.setOnItemClickListener(this);
        }
    }
    public String FolderFilter(){
        int buildVersion = Build.VERSION.SDK_INT;
        if (buildVersion > 22 )
            return "storage";
        else
            return "storage/udisk0";
    }


    public static void RecursionAddFile(String path, ArrayList<HashMap<String, Object>> result) {
        try {
            int level = 0;
            for (int i = 0; i < path.length(); i++)
                if (File.separatorChar == path.charAt(i))
                    level++;
            if (level > 3)
                return;
            File[] files = new File(path).listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isDirectory()) {
                        RecursionAddFile(file.getAbsolutePath(), result);
                    } else if (file.getName().endsWith(".bin")) {
                        HashMap<String, Object> temp = new HashMap<String, Object>();
                        temp.put(SettingsManager.STRING_NAME, file.getAbsolutePath());
                        result.add(temp);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onItemClick (AdapterView<?> parent, View view, int position, long id) {
        switch (mTag) {
            case OptionUiManager.OPTION_AUDIO_TRACK:
                getSettingsManager().setAudioTrack(position);
                break;
            case OptionUiManager.OPTION_DEFAULT_LANGUAGE:
                getSettingsManager().setDefLanguage(position);
                break;
            case OptionUiManager.OPTION_FBC_UPGRADE:
                listitem = optionListData.get(position).get(SettingsManager.STRING_NAME).toString();
                AlertDialog.Builder mbuiler = new Builder(mContext,AlertDialog.THEME_HOLO_LIGHT);
                mbuiler.setTitle("Warning!");
                mbuiler.setMessage("Please do not power off during the upgrade.");
                mbuiler.setPositiveButton("Sure",mOnClickListener);
                mbuiler.setNegativeButton("Cancel", mOnClickListener);
                AlertDialog mdialog = mbuiler.create();
                mdialog.show();
                Window window = mdialog.getWindow();
                WindowManager.LayoutParams lp = window.getAttributes();
                lp.alpha = 0.8f;
                window.setAttributes(lp);
                break;
            case OptionUiManager.OPTION_AD_LIST:
                getSettingsManager().setAudioADTrack(Integer.parseInt(optionListData.get(position).get(SettingsManager.STRING_PRIVATE).toString()));
                break;
        }
        ((TvSettingsActivity) mContext).getCurrentFragment().refreshList();
    }

    OnClickListener mOnClickListener = new OnClickListener() {
        public void onClick(DialogInterface dialog, int which) {
            switch (which) {
              case Dialog.BUTTON_POSITIVE:
                   mTvControlManager = TvControlManager.getInstance();
                   mTvControlManager.SetUpgradeFBCListener(OptionListLayout.this);
                   mTvControlManager.StartUpgradeFBC(listitem,0, 0x10000);
              break;
              case Dialog.BUTTON_NEGATIVE:
                   Toast.makeText(mContext, "Upgrade cancelled!", Toast.LENGTH_LONG).show();
              break;
            }
        }
    };

    public void onUpgradeStatus(int paramInt1, int paramInt2)
    {
        switch (paramInt2) {
        case -1:
            Toast.makeText(mContext, "Please check the connection of serial port!", Toast.LENGTH_LONG).show();
            break;
        case -2:
            Toast.makeText(mContext, "Sorry,Open file failure!", Toast.LENGTH_LONG).show();
            break;
        case -3:
            Toast.makeText(mContext, "Sorry,file size error!", Toast.LENGTH_LONG).show();
            break;
        case -4:
            Toast.makeText(mContext, "Sorry,read file error!", Toast.LENGTH_LONG).show();
            break;
        case -5:
            Toast.makeText(mContext, "Sorry,don't support upgrade mode!", Toast.LENGTH_LONG).show();
            break;
        case -6:
            Toast.makeText(mContext, "Sorry,upgrade block size error!", Toast.LENGTH_LONG).show();
            break;
        case -7:
            Toast.makeText(mContext, "Sorry,invalid upgrade file!", Toast.LENGTH_LONG).show();
            break;
        }
    }

    private SettingsManager getSettingsManager() {
        return ((TvSettingsActivity)mContext).getSettingsManager();
    }
}
