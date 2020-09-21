/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC GuideAdapter
 */

package com.droidlogic.droidlivetv.shortcut;

import android.content.Context;
import android.util.ArrayMap;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

import com.droidlogic.droidlivetv.R;

public class GuideAdapter extends BaseAdapter {
    private Context mContext;
    private LayoutInflater mInflater;
    ArrayList<ArrayMap<String, Object>> mlistItem = null;

    public GuideAdapter(Context context, ArrayList<ArrayMap<String, Object>> list) {
        mContext = context;
        this.mInflater = LayoutInflater.from(mContext);
        mlistItem = list;
    }

    public void refill(ArrayList<ArrayMap<String, Object>> list) {
        mlistItem.clear();
        mlistItem.addAll(list);
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mlistItem.size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        ViewHolder holder;

        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.layout_guide_double_text, null);
            holder = new ViewHolder();

            holder.playing = (ImageView) convertView.findViewById(R.id.img_playing);
            holder.appointed = (ImageView) convertView.findViewById(R.id.img_appointed);
            holder.time = (TextView) convertView.findViewById(R.id.text_time);
            holder.title = (TextView) convertView.findViewById(R.id.text_title);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder)convertView.getTag();
        }

        Object time_object =  mlistItem.get(position).get(GuideListView.ITEM_1);
        if (time_object != null) {
            holder.time.setText(time_object.toString());
        }
        Object title_object = mlistItem.get(position).get(GuideListView.ITEM_2);
        if (title_object != null) {
            holder.title.setText(title_object.toString());
        }
        Object watch_status_object = mlistItem.get(position).get(GuideListView.ITEM_5);
        if (watch_status_object != null) {
            String status = watch_status_object.toString();
            if (status.equals(GuideListView.STATUS_PLAYING)) {
                holder.playing.setImageResource(R.drawable.playing);
            } else if (status.equals(GuideListView.STATUS_APPOINTED_WATCH)) {
                holder.playing.setImageResource(R.drawable.appointed);
            } else {
                holder.playing.setImageResource(0);
            }
        }
        Object pvr_status_object = mlistItem.get(position).get(GuideListView.ITEM_6);
        if (pvr_status_object != null) {
            String status = pvr_status_object.toString();
            if (status.equals(GuideListView.STATUS_RECORDING)) {
                holder.appointed.setImageResource(R.drawable.recording_program);
            } else if (status.equals(GuideListView.STATUS_APPOINTED_RECORD)) {
                holder.appointed.setImageResource(R.drawable.scheduled_recording);
            } else {
                holder.appointed.setImageResource(0);
            }
        }
        return convertView;
    }

    private class ViewHolder {
        public ImageView playing;
        public ImageView appointed;
        public TextView time;
        public TextView title;
    }
}

