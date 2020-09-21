package com.droidlogic.fragment;

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.FocusFinder;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ListView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.util.AttributeSet;
import android.util.Log;
import android.text.TextUtils;

import com.droidlogic.fragment.dialog.DialogCallBack;
import com.droidlogic.fragment.ItemAdapter.ItemDetail;

import java.util.LinkedList;

import org.dtvkit.inputsource.R;
import org.droidlogic.dtvkit.DtvkitGlueClient;

public class ItemListView extends ListView implements OnItemSelectedListener {
    private static final String TAG = "ItemListView";

    public static final String ITEM_SATALLITE              = ParameterMananer.ITEM_SATALLITE;//"item_satallite";
    public static final String ITEM_TRANSPONDER            = ParameterMananer.ITEM_TRANSPONDER;//"item_tansponder";
    public static final String ITEM_SATALLITE_OPTION       = ParameterMananer.ITEM_SATALLITE_OPTION;//"item_satallite_option";
    public static final String ITEM_TRANSPONDER_OPTION     = ParameterMananer.ITEM_TRANSPONDER_OPTION;//"item_tansponder_option";
    public static final String ITEM_OPTION     = ParameterMananer.ITEM_OPTION;;

    private Context mContext;
    private int selectedPosition = 0;
    private String mListType = ITEM_SATALLITE;
    private ViewGroup rootView = null;
    private ListItemSelectedListener mListItemSelectedListener;
    private ListItemFocusedListener mListItemFocusedListener;
    private ListSwitchedListener mListSwitchedListener;
    private ListTypeSwitchedListener mListTypeSwitchedListener;
    private DialogCallBack mDataCallBack;

    private ParameterMananer mParameterMananer;

    public static final String LIST_LEFT = "left";
    public static final String LIST_RIGHT = "right";
    private String mCurrentListSide = LIST_LEFT;

    public ItemListView(Context context) {
        super(context);
        mContext = context;
        setRootView();
        mParameterMananer = new ParameterMananer(mContext, null);
    }
    public ItemListView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        setRootView();
        setOnItemSelectedListener(this);
        mParameterMananer = new ParameterMananer(mContext, null);
    }

    public void setDtvkitGlueClient(DtvkitGlueClient client) {
        mParameterMananer.setDtvkitGlueClient(client);
    }

    public void setCurrentListSide(String currentListSide) {
        mCurrentListSide = currentListSide;
    }

    public boolean dispatchKeyEvent (KeyEvent event) {
        Log.d(TAG, "dispatchKeyEvent mCurrentListSide = " + mCurrentListSide + ", event = " + event);
        if (KeyEvent.KEYCODE_BACK == event.getKeyCode()) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                Bundle bundle1 = new Bundle();
                bundle1.putString("action", "scan");
                bundle1.putInt("keycode", event.getKeyCode());
                mDataCallBack.onStatusChange(null, ParameterMananer.KEY_FUNCTION, bundle1);
            }
            return true;
        }
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_PROG_RED:
                case KeyEvent.KEYCODE_PROG_GREEN:
                case KeyEvent.KEYCODE_PROG_BLUE:
                case KeyEvent.KEYCODE_PROG_YELLOW:
                    if (mDataCallBack != null) {
                        Bundle bundle1 = new Bundle();
                        String function = "";
                        if (event.getKeyCode() == KeyEvent.KEYCODE_PROG_RED) {
                            function = "add";
                        } else if (event.getKeyCode() == KeyEvent.KEYCODE_PROG_GREEN) {
                            function = "edit";
                        } else if (event.getKeyCode() == KeyEvent.KEYCODE_PROG_BLUE) {
                            function = "scan";
                        } else if (event.getKeyCode() == KeyEvent.KEYCODE_PROG_YELLOW) {
                            function = "delete";
                        }
                        bundle1.putString("action", function);
                        bundle1.putInt("keycode", event.getKeyCode());
                        bundle1.putString("listtype", isLeftList() ? mParameterMananer.getCurrentListType() : ItemListView.ITEM_OPTION);
                        String parameter = "";
                        View selectedView1 = getSelectedView();
                        TextView textview = null;
                        if (selectedView1 != null) {
                            textview = (TextView) selectedView1.findViewById(R.id.textview_second);
                        }
                        if (isSatelliteList(mListType)) {
                            parameter = (textview != null && textview.getText() != null) ? textview.getText().toString() : "";//mParameterMananer.getCurrentSatellite();
                        } else if (isTransponderList(mListType)) {
                            parameter = (textview != null && textview.getText() != null) ? textview.getText().toString() : "";//mParameterMananer.getCurrentTransponder();
                        } else if (isRightList()) {
                            LinkedList<ItemDetail> satellitelist = mParameterMananer.getSatelliteList();
                            LinkedList<ItemDetail> transponderlist = mParameterMananer.getTransponderList();
                            String currentlist = mParameterMananer.getCurrentListType();
                            if ((TextUtils.equals(currentlist, ItemListView.ITEM_SATALLITE) && satellitelist != null && satellitelist.size() > 0) ||
                                    (TextUtils.equals(currentlist, ItemListView.ITEM_TRANSPONDER) && transponderlist != null && transponderlist.size() > 0)) {
                                if (!(event.getKeyCode() == KeyEvent.KEYCODE_PROG_BLUE)) {
                                    return true;
                                }
                            } else {
                                bundle1.putString("listtype", mParameterMananer.getCurrentListType());//need to add new device
                            }
                        }
                        bundle1.putString("parameter", parameter);
                        mDataCallBack.onStatusChange(getSelectedView(), ParameterMananer.KEY_FUNCTION, bundle1);
                    }
                    return true;
                case KeyEvent.KEYCODE_DPAD_UP:
                    if (selectedPosition == 0)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (selectedPosition == getAdapter().getCount() - 1)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    Log.d(TAG, "KEYCODE_DPAD_LEFT mListSwitchedListener = " + mListSwitchedListener + ", mListType = " + mListType);
                    if (isLeftList()) {
                        String savedListType = mParameterMananer.getCurrentListType();
                        String leftListType = TextUtils.isEmpty(savedListType) ? mListType : savedListType;
                        String result = switchListType(leftListType);
                        if (LIST_LEFT.equals(mCurrentListSide) && result != null) {
                            mListType = result;
                        }
                        if (mListTypeSwitchedListener != null && result != null) {
                            mListTypeSwitchedListener.onListTypeSwitched(result);
                        }
                        return true;
                    } else if (isRightList()) {
                        /*LinkedList<ItemDetail> satellitelist = mParameterMananer.getSatelliteList();
                        LinkedList<ItemDetail> transponderlist = mParameterMananer.getTransponderList();
                        String currentlist = mParameterMananer.getCurrentListType();
                        if (!((satellitelist != null && satellitelist.size() > 0) ||
                                (transponderlist != null && transponderlist.size() > 0))) {
                            Log.d(TAG, "no satellite or transponder!");
                            return true;
                        }*/
                        if (mListSwitchedListener != null) {
                            mListSwitchedListener.onListSwitched(LIST_LEFT);
                        }
                        return true;
                    }
                    return super.dispatchKeyEvent(event);
                    //break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    Log.d(TAG, "KEYCODE_DPAD_RIGHT mListSwitchedListener = " + mListSwitchedListener + ", mListType = " + mListType);
                    if (isLeftList() || isRightList()) {
                        if (mListSwitchedListener != null) {
                            mListSwitchedListener.onListSwitched(LIST_RIGHT);
                        }
                        return true;
                    }
                    return super.dispatchKeyEvent(event);
                    //break;
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                    if (isLeftList()) {
                        if (mListItemSelectedListener != null) {
                            mListItemSelectedListener.onListItemSelected(getSelectedItemPosition(), mListType);
                        }
                        return true;
                    } else if (isRightList()) {
                        if (mListItemSelectedListener != null) {
                            mListItemSelectedListener.onListItemSelected(getSelectedItemPosition(), mListType);
                        }
                        return true;
                    }
                    return super.dispatchKeyEvent(event);
                    //break;
                default:
                    break;
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
                        //clearChoosed(selectedView);
                    }
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }


    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        Log.d(TAG, "onItemSelected mCurrentListSide = " + mCurrentListSide + ", mListType = " + mListType +", position = " + position + ", id = " + id + ", view = " + view);
        selectedPosition = position;
        if (view != null) {
            /*if (hasFocus()) {
                setItemTextColor(view, true);
            } else {
                setItemTextColor(view, false);
            }*/
            setChoosed(view);
        }
        if (view != null && mListItemFocusedListener != null) {
            mListItemFocusedListener.onListItemFocused(/*(View)parent*/view, position, mListType);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    /*@Override
    protected void onFocusChanged (boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        Log.d(TAG, "onFocusChanged mListType = " + mListType + ", gainFocus = " + gainFocus +", direction = " + direction);
        View item = getSelectedView();
        if (item != null) {
            if (gainFocus) {
                cleanChoosed();
                //setItemTextColor(item, true);
                setChoosed(item);
            } else {
                //setItemTextColor(item, false);
                clearChoosed(item);
            }
        }
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
        /*RuntimeException e = new RuntimeException("cleanChoosed is here " + mListType);
        e.fillInStackTrace();
        Log.d(TAG, "@@@@@@@@ ", e);*/
        int color_text_item = mContext.getResources().getColor(R.color.common_item_background);
        for (int i = 0; i < getChildCount(); i ++) {
            View view = getChildAt(i);
            //Log.d(TAG, "cleanChoosed all view = " + view + ", i = " + i);
            view.setBackgroundColor(color_text_item);
        }
    }

    public void clearChoosed(View view) {
        /*RuntimeException e = new RuntimeException("clearChoosed is here " + mListType);
        e.fillInStackTrace();
        Log.d(TAG, "@@@@@@@@ ", e);*/
        int color_text_item = mContext.getResources().getColor(R.color.common_item_background);
        view.setBackgroundColor(color_text_item);
    }

    public void setChoosed(View view) {
        /*RuntimeException e = new RuntimeException("setChoosed is here " + mListType);
        e.fillInStackTrace();
        Log.d(TAG, "@@@@@@@@ ", e);*/
        int color_text_focused = mContext.getResources().getColor(R.color.common_focus);
        view.setBackgroundColor(color_text_focused);
    }

    public void setListType(String type) {
        mListType = type;
    }

    public void setListItemSelectedListener(ListItemSelectedListener l) {
        mListItemSelectedListener = l;
    }

    public void setListItemFocusedListener(ListItemFocusedListener l) {
        mListItemFocusedListener = l;
    }

    public void setListSwitchedListener(ListSwitchedListener l) {
        mListSwitchedListener = l;
    }

    public void setListTypeSwitchedListener(ListTypeSwitchedListener l) {
        mListTypeSwitchedListener = l;
    }

    public void setDataCallBack(DialogCallBack callback) {
        mDataCallBack = callback;
    }

    public interface ListItemSelectedListener {
        void onListItemSelected(int position, String type);
    }

    public interface ListItemFocusedListener {
        void onListItemFocused(View parent, int position, String type);
    }

    public interface ListSwitchedListener {
        void onListSwitched(String direction);
    }

    public interface ListTypeSwitchedListener {
        void onListTypeSwitched(String listtype);
    }

    public boolean isLeftList() {
        if ("left".equals(mParameterMananer.getCurrentListDirection())) {
            return true;
        }
        return false;
    }

    public boolean isRightList() {
        if ("right".equals(mParameterMananer.getCurrentListDirection())) {
            return true;
        }
        return false;
    }

    public boolean isSatelliteList(String type) {
        if (ITEM_SATALLITE.equals(type) && "left".equals(mParameterMananer.getCurrentListDirection())) {
            return true;
        }
        return false;
    }

    public boolean isTransponderList(String type) {
        if (ITEM_TRANSPONDER.equals(type) && "left".equals(mParameterMananer.getCurrentListDirection())) {
            return true;
        }
        return false;
    }

    private String switchListType(String leftListType) {
        String result = null;
        if (ITEM_SATALLITE.equals(leftListType)) {
            result = ITEM_TRANSPONDER;
        } else if (ITEM_TRANSPONDER.equals(leftListType)) {
            result = ITEM_SATALLITE;
        }
        return result;
    }
}


