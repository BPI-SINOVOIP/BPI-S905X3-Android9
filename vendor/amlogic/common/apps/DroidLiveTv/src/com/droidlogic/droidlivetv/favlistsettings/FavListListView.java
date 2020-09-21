package com.droidlogic.droidlivetv.favlistsettings;


import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.text.TextUtils;

import java.util.LinkedList;

public class FavListListView extends CustomedListView {
    private static final String TAG = FavListListView.class.getSimpleName();
    private static final boolean DEBUG = true;

    private KeyEventListener mKeyEventListener;
    private String mListType = null;

    public FavListListView(Context context) {
        this(context, null);
    }

    public FavListListView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public FavListListView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, defStyleAttr);
    }

    public FavListListView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes, FavListListView.class.getSimpleName());
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (DEBUG) {
            Log.d(TAG, "onKeyDown " + event);
        }
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:{
                if (mKeyEventListener != null/* && TextUtils.equals(mListType, SORT_EDIT_ALL_FAV_LIST)*/) {
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
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (DEBUG) {
            Log.d(TAG, "onKeyUp " + event);
        }
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_DPAD_UP:
            default:
                break;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {

        //super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        if (gainFocus) {
            super.onFocusChanged(false, direction, previouslyFocusedRect);
            View view = getChildAt(0);
            if  (view != null && view instanceof View) {
                setSelection(0);
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
        if (position >= firstVisiblePosition && position <= lastVisiblePosition) {
            View view = this.getChildAt(position - firstVisiblePosition);
            adapter.getView(position, view, this);
        }
    }

    public void updateAllItem(Context context, LinkedList<Item> data) {
        ItemAdapter adapter = (ItemAdapter)getAdapter();
        if (adapter == null) {
            adapter = new ItemAdapter(data, context, TAG);
            setAdapter(adapter);
            Log.d(TAG, "updateAllItem init adapter");
        }
        ((ItemAdapter)getAdapter()).setAllData(data);
        ((ItemAdapter)getAdapter()).notifyDataSetChanged();
    }
}
