package com.droidlogic.fragment.dialog;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.view.FocusFinder;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import com.droidlogic.fragment.ParameterMananer;
//import com.droidlogic.fragment.R;

import org.dtvkit.inputsource.R;

public class DialogItemListView extends ListView implements OnItemSelectedListener/*, AdapterView.OnItemClickListener, , View.OnFocusChangeListener*/ {
    private static final String TAG = "DialogItemListView";

    private Context mContext;
    private ViewGroup rootView = null;
    //private ListItemSelectedListener mListItemSelectedListener;
    //private ListItemFocusedListener mListItemFocusedListener;
    private String mTitle = null;
    private String mKey = null;
    private int selectedPosition = 0;
    private DialogCallBack mDialogCallBack = null;
    private CustomDialog.DialogUiCallBack mDialogUiCallBack = null;

    public DialogItemListView(Context context) {
        super(context);
        mContext = context;
        setRootView();
        setOnItemSelectedListener(this);
    }
    public DialogItemListView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        setRootView();
        setOnItemSelectedListener(this);
    }

    public void setDialogCallBack(DialogCallBack callback) {
        mDialogCallBack = callback;
    }

    public void setDialogUiCallBack(CustomDialog.DialogUiCallBack callback) {
        mDialogUiCallBack = callback;
    }

    public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                    if (selectedPosition == 0)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (selectedPosition == getAdapter().getCount() - 1)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    //if (ParameterMananer.KEY_UNICABLE.equals(getKey())) {
                        Bundle bundle1 = new Bundle();
                        bundle1.putString("action", event.getKeyCode() == KeyEvent.KEYCODE_DPAD_LEFT ? "left" : "right");
                        bundle1.putInt("position", getSelectedItemPosition());
                        bundle1.putString("title", mTitle);
                        bundle1.putString("key", mKey);
                        mDialogCallBack.onStatusChange(getSelectedView(), mKey, bundle1);
                        return true;
                    //}
                    //return super.dispatchKeyEvent(event);
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                    if (mDialogCallBack != null) {
                        int position = getSelectedItemPosition();
                        /*if ((ParameterMananer.KEY_SATALLITE.equals(getKey()) && position == 0) || (ParameterMananer.KEY_TRANSPONDER.equals(getKey()) && position == 1)) {
                            Log.d(TAG, "satellite or transponder no response");
                            return true;
                        }*/
                        Bundle bundle2 = new Bundle();
                        bundle2.putString("action", "selected");
                        bundle2.putInt("position", position);
                        bundle2.putString("title", mTitle);
                        bundle2.putString("key", mKey);
                        mDialogCallBack.onStatusChange(getSelectedView(), mKey, bundle2);
                    }
                    return true;
            }

            View selectedView = getSelectedView();
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if ( selectedView != null) {
                        clearChoosed(selectedView);
                    }
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if ( selectedView != null) {
                        //setItemTextColor(selectedView, false);
                    }
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }


    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        Log.d(TAG, "onItemSelected view = " + view +", position = " + position + ", id = " + id);
        selectedPosition = position;
        if (view != null) {
            setChoosed(view);
        }
        /*etDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
        if (view != null) {
            EditText edit = (EditText)view.findViewById(R.id.editText);
            if (edit != null) {
                edit.requestFocus();
            }
        }*/
        if (mDialogCallBack != null) {
            Bundle bundle = new Bundle();
            bundle.putString("action", "focused");
            bundle.putInt("position", position);
            bundle.putString("title", mTitle);
            bundle.putString("key", mKey);
            mDialogCallBack.onStatusChange(view, mKey, bundle);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
        Log.d(TAG, "onNothingSelected");
        /*setDescendantFocusability(ViewGroup.FOCUS_BEFORE_DESCENDANTS);*/
    }

    @Override
    protected void onFocusChanged (boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        Log.d(TAG, "onFocusChanged gainFocus = " + gainFocus +", direction = " + direction + ", previouslyFocusedRect = "+ previouslyFocusedRect);

    }

    /*@Override
    public void onFocusChange(View v, boolean hasFocus) {
        Log.d(TAG, "onFocusChange view = " + v +", hasFocus = " + hasFocus);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        Log.d(TAG, "onItemClick view = " + view +", position = " + position);
    }*/

    private void setItemTextColor (View view, boolean focused) {
        if (focused) {
            int color_text_focused = mContext.getResources().getColor(R.color.common_focus);
            for (int i = 0; i < ((ViewGroup)view).getChildCount(); i++) {
                View child = ((ViewGroup)view).getChildAt(i);
                if (child instanceof TextView) {
                    ((TextView)child).setTextColor(color_text_focused);
                }
            }
        } else {
            int color_text_item = mContext.getResources().getColor(R.color.common_item_background);
            for (int i = 0; i < ((ViewGroup)view).getChildCount(); i++) {
                View child = ((ViewGroup)view).getChildAt(i);
                if (child instanceof TextView) {
                    ((TextView)child).setTextColor(color_text_item);
                }
            }
        }
    }

    private void setRootView() {
        rootView = ((ViewGroup)((Activity)mContext).findViewById(android.R.id.content));
    }

    private boolean hasNextFocusView(int dec) {
        if (FocusFinder.getInstance().findNextFocus(rootView, this, dec) == null) {
            return false;
        } else {
            return true;
        }
    }

    public void cleanChoosed() {
        int color_text_item = mContext.getResources().getColor(R.color.common_item_background);
        for (int i = 0; i < getChildCount(); i ++) {
            View view = getChildAt(i);
            view.setBackgroundColor(color_text_item);
        }
    }

    public void clearChoosed(View view) {
        int color_text_item = mContext.getResources().getColor(R.color.common_item_background);
        view.setBackgroundColor(color_text_item);
    }

    public void setChoosed(View view) {
        int color_text_focused = mContext.getResources().getColor(R.color.common_focus);
        view.setBackgroundColor(color_text_focused);
    }

    public void setTitle(String title) {
        mTitle = title;
    }

    public void setKey(String key) {
        mKey = key;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getKey() {
        return mKey;
    }
    /*public void setListItemSelectedListener(ListItemSelectedListener l) {
        mListItemSelectedListener = l;
    }

    public void setListItemFocusedListener(ListItemFocusedListener l) {
        mListItemFocusedListener = l;
    }

    public interface ListItemSelectedListener {
        void onListItemSelected(int position, String type);
    }

    public interface ListItemFocusedListener {
        void onListItemFocused(View parent, int position, String type);
    }*/
}


