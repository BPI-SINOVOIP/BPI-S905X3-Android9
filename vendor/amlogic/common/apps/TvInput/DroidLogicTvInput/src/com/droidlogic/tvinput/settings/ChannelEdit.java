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

import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.Color;
import android.os.Message;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.view.LayoutInflater;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.widget.ImageView;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;

import com.droidlogic.tvinput.R;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;

public class ChannelEdit implements OnClickListener, OnFocusChangeListener, OnItemClickListener {
    private static final String TAG = "ChannelEdit";

    public static final int TYPE_ATV                           = 0;
    public static final int TYPE_DTV_TV                        = 1;
    public static final int TYPE_DTV_RADIO                     = 2;
    public static final int TYPE_DTV_MIX                       = 3;

    private static final int ACTION_INITIAL_STATE             = -1;
    private static final int ACTION_SHOW_LIST                 = 0;
    private static final int ACTION_SHOW_OPERATIONS          = 1;
    private static final int ACTION_OPERATIONS_EDIT          = 2;
    private static final int ACTION_OPERATIONS_SWAP          = 3;
    private static final int ACTION_OPERATIONS_MOVE          = 4;
    private static final int ACTION_OPERATIONS_SKIP          = 5;
    private static final int ACTION_OPERATIONS_DELETE        = 6;
    private static final int ACTION_OPERATIONS_FAVOURITE     = 7;

    private Context mContext;
    private View channelEditView = null;
    private TextView button_tv = null;
    private TextView button_radio = null;
    private OptionListView channelListView;
    private ViewGroup operationsView;
    private ViewGroup operationsEditView;
    MyAdapter ChannelAdapter;
    ArrayList<HashMap<String, Object>> ChannelListData = new ArrayList<HashMap<String, Object>>();

    private int channelType = TYPE_ATV;
    private int currentChannelPosition = ACTION_INITIAL_STATE;
    private int needOperateChannelPosition = ACTION_INITIAL_STATE;
    private int currentOperation = ACTION_INITIAL_STATE;


    public ChannelEdit (Context context, View view) {
        mContext = context;
        channelEditView = view;

        initChannelEditView();

    }

    private void initChannelEditView () {
        button_tv = (TextView)channelEditView.findViewById(R.id.channel_edit_tv);
        button_radio = (TextView)channelEditView.findViewById(R.id.channel_edit_radio);
        button_tv.setOnClickListener(this);
        button_tv.setOnFocusChangeListener(this);
        button_radio.setOnClickListener(this);
        button_radio.setOnFocusChangeListener(this);

        channelListView = (OptionListView)channelEditView.findViewById(R.id.channnel_edit_list);
        operationsView = (ViewGroup)channelEditView.findViewById(R.id.channel_edit_operations);
        operationsEditView = (ViewGroup)channelEditView.findViewById(R.id.channel_edit_editname);

        if (getSettingsManager().getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV
                || getSettingsManager().getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            channelType = TYPE_DTV_TV;
        } else if (getSettingsManager().getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            channelType = TYPE_ATV;
            ChannelListData = ((TvSettingsActivity)mContext).getSettingsManager().getChannelList(channelType);
        }

        ChannelAdapter = new MyAdapter(mContext, ChannelListData);
        channelListView.setAdapter(ChannelAdapter);
        channelListView.setOnItemClickListener(this);

        if (channelType == TYPE_DTV_TV) {
            channelListView.setVisibility(View.GONE);
            operationsView.setVisibility(View.GONE);
            operationsEditView.setVisibility(View.GONE);
        } else if (channelType == TYPE_ATV) {
            button_tv.setVisibility(View.GONE);
            button_radio.setVisibility(View.GONE);
            operationsView.setVisibility(View.GONE);
            operationsEditView.setVisibility(View.GONE);
        }

        TextView edit = (TextView)channelEditView.findViewById(R.id.edit);
        TextView ensure_edit = (TextView)channelEditView.findViewById(R.id.ensure_edit);
        TextView swap = (TextView)channelEditView.findViewById(R.id.swap);
        TextView move = (TextView)channelEditView.findViewById(R.id.move);
        TextView skip = (TextView)channelEditView.findViewById(R.id.skip);
        TextView delete = (TextView)channelEditView.findViewById(R.id.delete);
        TextView favourite = (TextView)channelEditView.findViewById(R.id.favourite);
        edit.setOnClickListener(this);
        edit.setOnFocusChangeListener(this);
        ensure_edit.setOnClickListener(this);
        ensure_edit.setOnFocusChangeListener(this);
        swap.setOnClickListener(this);
        swap.setOnFocusChangeListener(this);
        move.setOnClickListener(this);
        move.setOnFocusChangeListener(this);
        skip.setOnClickListener(this);
        skip.setOnFocusChangeListener(this);
        delete.setOnClickListener(this);
        delete.setOnFocusChangeListener(this);
        favourite.setOnClickListener(this);
        favourite.setOnFocusChangeListener(this);

        if (getSettingsManager().getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            //do not support this now. fix me.
            swap.setVisibility(View.GONE);
            move.setVisibility(View.GONE);
        }
    }

    private void showDtvMainView () {
        button_tv.setVisibility(View.VISIBLE);
        button_tv.requestFocus();
        button_radio.setVisibility(View.VISIBLE);

        channelListView.setVisibility(View.GONE);
        operationsView.setVisibility(View.GONE);
        operationsEditView.setVisibility(View.GONE);
    }

    private void showListView () {
        if (currentOperation != ACTION_INITIAL_STATE) {
            channelListView.setSelector(R.anim.flicker_background);
            AnimationDrawable animaition = (AnimationDrawable)channelListView.getSelector();
            animaition.start();
        }

        freshChannelList();
        channelListView.setVisibility(View.VISIBLE);
        channelListView.requestFocus();
        channelListView.setNextFocusLeftId(R.id.content_list);

        button_tv.setVisibility(View.GONE);
        button_radio.setVisibility(View.GONE);
        operationsView.setVisibility(View.GONE);
        operationsEditView.setVisibility(View.GONE);
    }

    private void showOperationsView () {
        operationsView.setVisibility(View.VISIBLE);
        TextView edit = (TextView)channelEditView.findViewById(R.id.edit);
        edit.requestFocus();
        channelListView.setVisibility(View.GONE);
        setFocus(operationsView);
    }

    private void showEditView () {
        operationsEditView.setVisibility(View.VISIBLE);
        EditText edit_name = (EditText)channelEditView.findViewById(R.id.edit_name);
        edit_name.requestFocus();
        operationsView.setVisibility(View.GONE);
        setFocus(operationsEditView);
    }

    private void setFocus(View view) {
        View firstFocusableChild = null;
        View lastFocusableChild = null;
        for (int i = 0; i < ((ViewGroup)view).getChildCount(); i++) {
            View child = ((ViewGroup)view).getChildAt(i);
            if (child != null && child.hasFocusable() ) {
                if (firstFocusableChild == null) {
                    firstFocusableChild = child;
                }
                child.setNextFocusLeftId(R.id.content_list);
                lastFocusableChild = child;
            }
        }

        if (firstFocusableChild != null && lastFocusableChild != null) {
            firstFocusableChild.setNextFocusUpId(firstFocusableChild.getId());
            lastFocusableChild.setNextFocusDownId(lastFocusableChild.getId());
        }
    }

    private void setChannelName () {
        EditText edit_name = (EditText)channelEditView.findViewById(R.id.edit_name);
        getSettingsManager().setChannelName(channelType, currentChannelPosition, edit_name.getText().toString());
        edit_name.setText("");
    }

    private void swapChannelPosition () {
        getSettingsManager().swapChannelPosition(channelType, needOperateChannelPosition, currentChannelPosition);
    }

    private void moveChannelPosition () {
        getSettingsManager().moveChannelPosition(channelType, needOperateChannelPosition, currentChannelPosition);
    }

    private void skipChannel () {
        getSettingsManager().skipChannel(channelType, currentChannelPosition);
    }

    private void deleteChannel () {
        getSettingsManager().deleteChannel(channelType, currentChannelPosition);
    }

    private void setFavouriteChannel () {
        getSettingsManager().setFavouriteChannel(channelType, currentChannelPosition);
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.channel_edit_tv:
                channelType = TYPE_DTV_TV;
                showListView();
                break;
            case R.id.channel_edit_radio:
                channelType = TYPE_DTV_RADIO;
                showListView();
                break;
            case R.id.edit:
                showEditView();
                break;
            case R.id.ensure_edit:
                setChannelName();
                showListView();
                break;
            case R.id.swap:
                needOperateChannelPosition = currentChannelPosition;
                currentOperation = ACTION_OPERATIONS_SWAP;
                showListView();
                break;
            case R.id.move:
                needOperateChannelPosition = currentChannelPosition;
                currentOperation = ACTION_OPERATIONS_MOVE;
                showListView();
                break;
            case R.id.skip:
                skipChannel();
                showListView();
                break;
            case R.id.delete:
                deleteChannel();
                showListView();
                break;
            case R.id.favourite:
                setFavouriteChannel();
                showListView();
                break;
        }
    }

    @Override
    public void onFocusChange (View v, boolean hasFocus) {
        if (v instanceof TextView) {
            if (hasFocus) {
                ((TextView)v).setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
            } else
                ((TextView)v).setTextColor(mContext.getResources().getColor(R.color.color_text_item));
        }
    }

    @Override
    public void onItemClick (AdapterView<?> parent, View view, int position, long id) {
        if (ChannelListData.get(position).get(SettingsManager.STRING_NAME).toString()
            .equals(mContext.getResources().getString(R.string.error_no_channel))) {
            return;
        }
        currentChannelPosition = position;
        if (currentOperation == ACTION_INITIAL_STATE) {
            showOperationsView();
        } else {
            if (currentOperation == ACTION_OPERATIONS_SWAP)
                swapChannelPosition();
            else if (currentOperation == ACTION_OPERATIONS_MOVE)
                moveChannelPosition();
            channelListView.setSelector(R.drawable.item_background);
            freshChannelList();
        }
        recoverActionState();
    }

    private void freshChannelList () {
        ArrayList<HashMap<String, Object>> list = null;

        ChannelListData.clear();
        list = ((TvSettingsActivity)mContext).getSettingsManager().getChannelList(channelType);

        if (list != null) {
            ChannelListData.addAll(list);
            ChannelAdapter.notifyDataSetChanged();

            if (!ChannelListData.get(0).get(SettingsManager.STRING_NAME).toString()
                .equals(mContext.getResources().getString(R.string.error_no_channel))) {
                channelListView.setOnItemClickListener(this);
            } else {
                channelListView.setOnItemClickListener(null);
            }
        }
    }

    private void recoverActionState () {
        currentOperation = ACTION_INITIAL_STATE;
        needOperateChannelPosition = ACTION_INITIAL_STATE;
    }

    private SettingsManager getSettingsManager() {
        return ((TvSettingsActivity)mContext).getSettingsManager();
    }

    private class MyAdapter extends BaseAdapter {
        private Context mContext;
        private LayoutInflater mInflater;
        ArrayList<HashMap<String, Object>> mlistItem = null;

        public MyAdapter(Context context, ArrayList<HashMap<String, Object>> list) {
            mContext = context;
            this.mInflater = LayoutInflater.from(mContext);
            mlistItem = list;
        }

        @Override
        public int getCount() {
            return mlistItem.size();
        }

        @Override
        public Object getItem(int position) {
            return mlistItem.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(final int position, View convertView, ViewGroup parent) {
            ViewHolder holder;

            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.layout_option_icon_text, null);
                holder = new ViewHolder();

                holder.icon = (ImageView) convertView.findViewById(R.id.image_icon);
                holder.name = (TextView) convertView.findViewById(R.id.text_name);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder)convertView.getTag();
            }

            Object icon_object =  mlistItem.get(position).get(SettingsManager.STRING_ICON);
            if (icon_object != null) {
                holder.icon.setImageDrawable(mContext.getResources().getDrawable((int)icon_object));
            } else {
                holder.icon.setImageDrawable(null);
            }
            Object name_object = mlistItem.get(position).get(SettingsManager.STRING_NAME);
            if (name_object != null) {
                holder.name.setText(name_object.toString());
            }
            return convertView;
        }

        private class ViewHolder {
            public ImageView icon;
            public TextView name;
        }
    }
}
