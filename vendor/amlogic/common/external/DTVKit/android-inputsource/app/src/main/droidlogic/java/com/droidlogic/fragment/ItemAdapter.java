package com.droidlogic.fragment;

import java.util.LinkedList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import org.dtvkit.inputsource.R;

public class ItemAdapter extends BaseAdapter {

    private LinkedList<ItemDetail> mData;
    private Context mContext;

    public ItemAdapter(LinkedList<ItemDetail> mData, Context mContext) {
        this.mData = mData;
        this.mContext = mContext;
    }

    public void reFill(LinkedList<ItemDetail> data) {
        mData.clear();
        mData.addAll(data);
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mData.size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if (convertView == null) {
            if (mData.get(position).isSatelliteList()) {
                convertView = LayoutInflater.from(mContext).inflate(R.layout.list_item_satellite,parent,false);
            } else {
                convertView = LayoutInflater.from(mContext).inflate(R.layout.list_item_parameter,parent,false);
            }
            holder = new ViewHolder();
            holder.select = (TextView) convertView.findViewById(R.id.item_select);
            holder.textview2 = (TextView) convertView.findViewById(R.id.textview_second);
            holder.leftarray = (TextView) convertView.findViewById(R.id.left_array_text);
            holder.textview3 = (TextView) convertView.findViewById(R.id.textview_third);
            holder.rightarray = (TextView) convertView.findViewById(R.id.right_array_text);
            convertView.setTag(holder);
        } else{
            holder = (ViewHolder) convertView.getTag();
        }

        int editstatus = mData.get(position).getEditStatus();
        if (editstatus == ItemDetail.NONE_EDIT) {
            holder.select.setVisibility(View.INVISIBLE);
            holder.leftarray.setVisibility(View.GONE);
            holder.rightarray.setVisibility(View.GONE);
        } else if (editstatus == ItemDetail.SWITCH_EDIT) {
            holder.select.setVisibility(View.INVISIBLE);
            holder.leftarray.setVisibility(View.VISIBLE);
            holder.rightarray.setVisibility(View.VISIBLE);
        } else {
            holder.select.setVisibility(View.VISIBLE);
            holder.leftarray.setVisibility(View.GONE);
            holder.rightarray.setVisibility(View.GONE);
            if (editstatus == ItemDetail.SELECT_EDIT) {
                holder.select.setBackgroundResource(R.drawable.ic_check_white_24dp);
            } else {
                holder.select.setVisibility(View.INVISIBLE);
            }
        }
        holder.textview2.setText(mData.get(position).getFirstText());
        if (mData.get(position).getSecondText() == null) {
            holder.textview3.setVisibility(View.GONE);
        } else {
            holder.textview3.setVisibility(View.VISIBLE);
            holder.textview3.setText(mData.get(position).getSecondText());
        }
        if (mData.get(position).isSatelliteList()) {
            holder.leftarray.setVisibility(View.GONE);
            holder.rightarray.setVisibility(View.GONE);
            holder.textview3.setVisibility(View.GONE);
        }
        return convertView;
    }

    public static class ItemDetail {
        private int mEditStatus = 0;
        private String mFirstText;
        private String mSecondText;
        private boolean mIsSatellite = true;

        public static final int NONE_EDIT = 0;
        public static final int SELECT_EDIT = 1;
        public static final int NOT_SELECT_EDIT = 2;
        public static final int SWITCH_EDIT = 3;

        public ItemDetail() {
        }

        public ItemDetail(int editstatus, String first, String second, boolean isSatellite) {
            this.mEditStatus = editstatus;
            this.mFirstText = first;
            this.mSecondText = second;
            this.mIsSatellite = isSatellite;
        }

        public int getEditStatus() {
            return mEditStatus;
        }

        public String getFirstText() {
            return mFirstText;
        }

        public String getSecondText() {
            return mSecondText;
        }

        public void setEditStatus(int editstatus) {
            this.mEditStatus = editstatus;
        }

        public void setFirstText(String first) {
            this.mFirstText = first;
        }

        public void setSecondText(String second) {
            this.mSecondText = second;
        }

        public boolean isSatelliteList() {return this.mIsSatellite;}
    }

    public class ViewHolder{
        TextView select;
        TextView textview2;
        TextView leftarray;
        TextView textview3;
        TextView rightarray;
    }
}