package com.droidlogic.droidlivetv.favlistsettings;


import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;

import java.util.LinkedList;

public class ChannelListListView extends CustomedListView {
    private static final String TAG = ChannelListListView.class.getSimpleName();

    private KeyEventListener mKeyEventListener;
    private String mListType = null;

    public ChannelListListView(Context context) {
        this(context, null);
    }

    public ChannelListListView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ChannelListListView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, defStyleAttr);
    }

    public ChannelListListView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes, ChannelListListView.class.getSimpleName());
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:{
                if (mKeyEventListener != null) {
                    Bundle bundle = new Bundle();
                    bundle.putInt(KEY_ACTION_CODE, keyCode);
                    bundle.putString(KEY_LIST_TYPE, mListType);
                    ItemAdapter adapter = (ItemAdapter)this.getAdapter();
                    Item item = null;
                    if (adapter != null && adapter.getCount() > 0) {
                        item = adapter.getItem(this.getSelectedItemPosition());
                    }
                    mKeyEventListener.onKeyEventCallbak(item, bundle);
                    return true;
                }
                break;
            }
            default:
                break;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        int lastSelectItem = getSelectedItemPosition();
        //super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        if (gainFocus) {
            super.onFocusChanged(false, direction, previouslyFocusedRect);
            //setSelection(lastSelectItem);
            //ChannelListItem item = (ChannelListItem) getSelectedItem();
            View view = getChildAt(lastSelectItem);
            if  (view != null && view instanceof View) {
                view.requestFocus();
            }
        } else {
            super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        }
    }

    public void setKeyEventListener(KeyEventListener listener) {
        mKeyEventListener = listener;
    }

    public void setListType(String type) {
        mListType = type;
    }

    public void updateItem(int position, Item item) {
        ItemAdapter adapter = (ItemAdapter)this.getAdapter();
        if (adapter == null) {
            Log.d(TAG, "updateItem null return");
            return;
        }
        adapter.setDataByPosition(position,item);
        int firstVisiblePosition = this.getFirstVisiblePosition();
        int lastVisiblePosition = this.getLastVisiblePosition();
        //Log.d(TAG, "updateItem position = " + position + ", firstVisiblePosition = " + firstVisiblePosition + ", lastVisiblePosition = " + lastVisiblePosition);
        if (position >= firstVisiblePosition && position <= lastVisiblePosition) {
            View view = this.getChildAt(position - firstVisiblePosition);
            adapter.getView(position, view, this);
        }
    }

    public void updateAllItem(Context context, LinkedList<Item> data) {
        ItemAdapter adapter = (ItemAdapter)this.getAdapter();
        if (adapter == null) {
            adapter = new ItemAdapter(data, context, TAG);
            setAdapter(adapter);
            Log.d(TAG, "updateAllItem init adapter");
        }
        ((ItemAdapter)getAdapter()).setAllData(data);
        ((ItemAdapter)getAdapter()).notifyDataSetChanged();
    }
}
