package com.droidlogic.droidlivetv.favlistsettings;

import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import com.droidlogic.droidlivetv.R;

public abstract class Item {
    private static final String TAG = "Item";
    private String mItemName;

    public static final int ACTION_CHANNEL_SORT_ALL = 0;
    public static final int ACTION_CHANNEL_SORT_AZ = 1;
    public static final int ACTION_CHANNEL_SORT_TP = 2;
    public static final int ACTION_CHANNEL_SORT_NETWORKID = 3;
    public static final int ACTION_CHANNEL_SORT_SORT = 4;
    public static final int ACTION_FUNVTION_FIND = 5;
    public static final int ACTION_FUNVTION_ADD_FAV = 6;
    public static final int ACTION_FUNVTION_SATELLITE = 7;
    public static final int ACTION_FUNVTION_FAVLIST = 8;

    public static final int CONTAINER_ITEM_ALL_CHANNEL = 0;
    public static final int CONTAINER_ITEM_SORT_CONTENT = 1;
    public static final int CONTAINER_ITEM_SORT_KEY = 2;
    public static final int CONTAINER_ITEM_ALL_FAV = 3;
    public static final int CONTAINER_ITEM_CHANNEL_FAV = 4;

    public static final String LIST_ALL_FAV_LIST = "all_fav_list";
    public static final String LIST_CHANNEL_FAV_LIST = "all_channel_fav_list";
    public static final String LIST_EDIT_ALL_FAV_LIST = "edit_fav_list";

    public Item(String name) {
        this.mItemName = name;
    }

    protected abstract int getResourceId();
    protected abstract String getTitle();
    protected abstract boolean needTitle();
    protected abstract Drawable getDrawable();
    protected abstract boolean needIconDrawable();
    protected abstract String getLayoutIdInfo();
    protected abstract JSONObject getJSONObject();
    protected abstract boolean isNeedShowIcon();
    protected abstract String getOriginTitle();

    protected String getStringValueFromJSONObject(String key) {
        String result = null;
        JSONObject obj = getJSONObject();
        if (!TextUtils.isEmpty(key) && obj != null && obj.length() > 0) {
            try {
                String value = obj.getString(key);
                result = value;
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
        return result;
    }

    protected int getIntValueFromJSONObject(String key) {
        int result = -1;
        JSONObject obj = getJSONObject();
        if (!TextUtils.isEmpty(key) && obj != null && obj.length() > 0) {
            try {
                int value = obj.getInt(key);
                result = value;
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
        return result;
    }

    protected int getTitleId() {
        return R.id.title_text;
    }

    protected  int getIconTextId() {
        return R.id.icon_text;
    }

    protected String addTextView(int resId, String value) {
        String result = null;
        JSONObject obj = new JSONObject();
        try {
            obj.put("type", TextView .class);
            obj.put("resId", resId);
            obj.put("value", value);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (obj != null && obj.length() > 0) {
            result = obj.toString();
        }
        return result;
    }

    protected String addIconTextView(int resId, int iconRes) {
        String result = null;
        JSONObject obj = new JSONObject();
        try {
            obj.put("type", TextView .class);
            obj.put("resId", resId);
            obj.put("iconRes", iconRes);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (obj != null && obj.length() > 0) {
            result = obj.toString();
        }
        return result;
    }
}
