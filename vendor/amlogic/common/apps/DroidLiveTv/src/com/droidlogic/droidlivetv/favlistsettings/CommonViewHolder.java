package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class CommonViewHolder {
    private static final String TAG = CommonViewHolder.class.getSimpleName();

    private SparseArray<View> mViews;
    private int mPosition;
    private View mConvertView;

    public CommonViewHolder(Context context, ViewGroup parent, int layoutId, int position) {
        this.mPosition = position;
        this.mViews = new SparseArray<View>();
        mConvertView = LayoutInflater.from(context).inflate(layoutId, parent, false);
        mConvertView.setTag(this);
    }

    public View getConvertView() {
        return mConvertView;
    }

    public <T extends View> T getView(int viewId) {
        View view = mViews.get(viewId);
        if (view == null) {
            view = mConvertView.findViewById(viewId);
            mViews.put(viewId, view);
        }
        return (T) view;
    }

    public CommonViewHolder setText(int viewId, String text) {
        View view = getView(viewId);
        if (view instanceof TextView) {
            TextView textView = (TextView)view;
            if (textView != null) {
                textView.setText(text);
            }
        }
        return this;
    }

    public CommonViewHolder setIcon(int viewId, Drawable draw) {
        //Log.d(TAG, "setIcon viewId = " + viewId + ", draw = " + draw);
        View view = getView(viewId);
        if (view instanceof TextView) {
            TextView textView = (TextView)view;
            if (textView != null) {
                if (draw != null) {
                    //textView.setText("icon");
                    //textView.setCompoundDrawables(draw, null, null, null);
                    textView.setCompoundDrawablesWithIntrinsicBounds(null, null, draw, null);
                } else {
                    //textView.setText("null");
                    textView.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
                }
            }
        }
        return this;
    }

    public static CommonViewHolder get(Context context, View convertView, ViewGroup parent, int layoutId, int position) {
        if (convertView == null) {
            return new CommonViewHolder(context, parent, layoutId, position);
        } else {
            CommonViewHolder viewHolder = (CommonViewHolder) convertView.getTag();
            viewHolder.mPosition = position;
            return viewHolder;
        }
    }
}
