/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tv.droidlogic.subtitleui;

import android.content.Context;
import android.util.Log;
import android.text.InputType;
import android.os.Bundle;
import android.view.KeyEvent;
import android.media.tv.TvTrackInfo;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.app.AlertDialog;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Button;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.text.TextUtils;
import android.widget.LinearLayout;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;

import com.android.tv.ui.sidepanel.SwitchItem;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.droidlogic.channelui.EditTextItem;
import com.android.tv.util.CaptionSettings;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.DividerItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.util.CaptionSettings;
import com.android.tv.data.api.Channel;
import com.android.tv.MainActivity;

import java.util.List;
import java.util.ArrayList;

import com.android.tv.R;

public class TeleTextSettingFragment extends SideFragment {
    private static final String TRACKER_LABEL ="open teletext" ;
    private static final String TAG ="TeleTextSettingFragment" ;
    private List<Item> mItems;
    private boolean mInitSubtitleSwitchOn = false;

    private static final int MIN_PAGE_NUMBER = 100;
    private static final int MAX_PAGE_NUMBER = 899;

    private static final int[] COUNTRY_LIST_RES =  {R.string.subtitle_country_english, R.string.subtitle_country_deutsch, R.string.subtitle_country_svenska,
                                                                       R.string.subtitle_country_italiano, R.string.subtitle_country_france, R.string.subtitle_country_portugal,
                                                                       R.string.subtitle_country_cesky, R.string.subtitle_country_turkey, R.string.subtitle_country_ellinika,
                                                                       R.string.subtitle_country_alarabia, R.string.subtitle_country_russky, R.string.subtitle_country_hebrew};
    private static final int[] REGION_ID_MAPS = {16, 17, 18, 19, 20, 21, 14, 22, 55 , 64, 36, 80};

    public TeleTextSettingFragment() {
        super(KeyEvent.KEYCODE_CAPTIONS, KeyEvent.KEYCODE_S);
    }

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.subtitle_teletext_settings);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {

        mItems = new ArrayList<>();

        mItems.add(new DividerItem(getString(R.string.subtitle_teletext_page_turn)));
        mItems.add(new ActionItem(getString(R.string.subtitle_teletext_page_up)) {
            @Override
            protected void onSelected() {
                Bundle bundle = new Bundle();
                bundle.putBoolean(ACTION_PAGE_UP,true);
                sendCommandByTif(ACTION_PAGE_UP, bundle);
            }
        });
        mItems.add(new ActionItem(getString(R.string.subtitle_teletext_page_down)) {
            @Override
            protected void onSelected() {
                Bundle bundle = new Bundle();
                bundle.putBoolean(ACTION_PAGE_DOWN,true);
                sendCommandByTif(ACTION_PAGE_DOWN, bundle);
            }
        });

        mItems.add(new DividerItem(getString(R.string.subtitle_teletext_page_setting)));
        mItems.add(new ActionItem(getString(R.string.subtitle_teletext_page_number)) {
            @Override
            protected void onSelected() {
                createEditDialogUi(-1);
            }
        });
        final MainActivity mainActivity = getMainActivity();
        boolean isDtvKit = false;
        if (mainActivity != null) {
            Channel channel = mainActivity.getCurrentChannel();
            if (channel != null) {
                String inputId = channel.getPackageName();
                if (TextUtils.equals(TeleTextAdvancedSettings.DTVKIT_PACKAGE, inputId)) {
                    isDtvKit = true;
                }
            }
        }
        if (isDtvKit) {
            mItems.add(new DividerItem(getString(R.string.subtitle_teletext_advanced_function)));
            mItems.add(new ActionItem(getString(R.string.subtitle_teletext_advanced_settings)) {
                @Override
                protected void onSelected() {
                    if (mainActivity != null) {
                        mainActivity.getOverlayManager().getSideFragmentManager().hideAll(false);
                        mainActivity.showTeleTextAdvancedSettings();
                    }
                }
            });
        } else {
            mItems.add(new DividerItem(getString(R.string.subtitle_teletext_page_cuntry)));
            for (int i = 0; i < COUNTRY_LIST_RES.length; i++) {
                mItems.add(new PageTurningItem(getString(COUNTRY_LIST_RES[i]), i));
            }
        }

        return mItems;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if (SubtitleFragment.getCaptionsEnabled(getMainActivity())) {
            mInitSubtitleSwitchOn = true;
        }
        Bundle bundle = new Bundle();
        bundle.putBoolean(ACTION_TELETEXT_START, true);
        sendCommandByTif(ACTION_TELETEXT_START, bundle);
        Log.d(TAG, "start teletext");
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        Bundle bundle = new Bundle();
        bundle.putBoolean(ACTION_TELETEXT_START, false);
        sendCommandByTif(ACTION_TELETEXT_START, bundle);
        Log.d(TAG, "stop teletext");
        if (mInitSubtitleSwitchOn) {
            getMainActivity().selectSubtitleTrack(CaptionSettings.OPTION_ON, getMainActivity().getSelectedTrack(TvTrackInfo.TYPE_SUBTITLE));
        }
    }

    private static final int OPTION_PAGE_UP = 0;
    private static final int OPTION_PAGE_DOWN = 1;
    private static final int OPTION_PAGE_NUMBER = 2;

    private class PageTurningItem extends RadioButtonItem {
        private final int mOption;

        private PageTurningItem(String title, int option) {
            super(title);
            mOption = option;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            Bundle bundle = new Bundle();
            bundle.putInt(ACTION_TELETEXT_COUNTRY_SET, REGION_ID_MAPS[mOption]);
            sendCommandByTif(ACTION_TELETEXT_COUNTRY_SET, bundle);
            //closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
        }
    }

    private AlertDialog mAlertDialog;

    private void createEditDialogUi (int number) {
        LayoutInflater inflater = (LayoutInflater)getMainActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.option_item_edit_dialog, null);

        AlertDialog.Builder builder = new AlertDialog.Builder(getMainActivity());
        mAlertDialog = builder.create();
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        mAlertDialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
        mAlertDialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);

        TextView title = (TextView)view.findViewById(R.id.title);
        Button button_ok = (Button)view.findViewById(R.id.ok);
        EditText edit_text = (EditText)view.findViewById(R.id.editText);
        edit_text.requestFocus();
        title.setText(R.string.subtitle_teletext_page_setting);
        getMainActivity().getOverlayManager().getSideFragmentManager().closeScheduleHideAll();
        Log.d(TAG, "cancle timeout");
        button_ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (edit_text.getText() != null && edit_text.getText().length() > 0) {
                     String text = edit_text.getText().toString();
                     if (isNumeric(text)) {
                         Bundle bundle = new Bundle();
                         bundle.putInt(ACTION_PAGE_NUMBER, Integer.parseInt(text));
                         sendCommandByTif(ACTION_PAGE_NUMBER, bundle);
                     }
                     mAlertDialog.dismiss();
                }
            }
        });
        mAlertDialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "start timeout");
                getMainActivity().getOverlayManager().getSideFragmentManager().scheduleHideAll();
            }
        });
    }

    private boolean isNumeric(String str) {
        if (str == null || str.length() == 0) {
            return false;
        }
        for (int i = 0; i < str.length(); i++) {
            if (!Character.isDigit(str.charAt(i))) {
                return false;
            }
        }
        return true;
    }

    private static final String ACTION_PAGE_UP = "action_teletext_up";
    private static final String ACTION_PAGE_DOWN = "action_teletext_down";
    private static final String ACTION_PAGE_NUMBER = "action_teletext_number";
    private static final String ACTION_TELETEXT_START = "action_teletext_start";
    private static final String ACTION_TELETEXT_COUNTRY_SET = "action_teletext_country";

    private void sendCommandByTif(String action, Bundle bundle) {
        getMainActivity().getTvView().sendAppPrivateCommand(action, bundle);
    }
}
