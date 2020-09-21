package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import java.util.LinkedList;

public abstract class CommonAdapter<T> extends BaseAdapter {
    private static final String TAG = "CommonAdapter";

    private LinkedList<T> mData;
    private Context mContext;
    private String mTitle;

    public CommonAdapter(LinkedList<T> mData, Context mContext, String title) {
        this.mData = mData;
        this.mContext = mContext;
        this.mTitle = title;
    }

    @Override
    public int getCount() {
        return mData.size();
    }

    @Override
    public T getItem(int position) {
        return mData.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setDataByPosition(int position, T item) {
        if (mData != null && mData.size() >= position) {
            mData.set(position,item );
        }
    }

    public void setAllData(LinkedList<T> data) {
        mData = data;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        T item = mData.get(position);
        CommonViewHolder viewHolder = setViewContent(mContext, mTitle, convertView, parent, position, item);
        return viewHolder.getConvertView();
    }

    protected abstract CommonViewHolder setViewContent(Context context, String title, View view, ViewGroup parent, int position, T t);
}
