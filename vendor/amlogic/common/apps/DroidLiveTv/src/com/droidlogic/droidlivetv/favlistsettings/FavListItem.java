package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.droidlogic.droidlivetv.R;

public class FavListItem extends Item {
    private static final String TAG = FavListItem.class.getSimpleName();

    private Context mContext;
    private String mName;
    private boolean needShowIcon = false;
    private String mInfoJson;
    private JSONObject mJSONObject;

    public FavListItem(Context context, String name, boolean show, String objJson) {
        super(FavListItem.class.getSimpleName());
        this.mContext = context;
        this.mName = name;
        this.needShowIcon = show;
        this.mInfoJson = objJson;
        this.initJSONObject(objJson);
    }

    private void initJSONObject(String json) {
        try {
            if (!TextUtils.isEmpty(json)) {
                mJSONObject =  new JSONObject(json);
            }
        } catch (JSONException e) {
            Log.e(TAG, "initJSONObject e = " + e.getMessage());
            e.printStackTrace();
        }
    }

    public void setJSONObject(JSONObject obj) {
        mJSONObject = obj;
    }

    public String getOriginTitle() {
        String result = null;
        try {
            if (mJSONObject != null) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_NAME);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return result;
    }

    public void setNeedShowIcon(boolean value) {
        needShowIcon = value;
    }

    @Override
    protected JSONObject getJSONObject() {
        return mJSONObject;
    }

    @Override
    protected int getResourceId() {
        return R.layout.item_favlist_layout;
    }

    @Override
    protected String getTitle() {
        return mName;
    }

    @Override
    protected boolean needTitle() {
        return true;
    }

    @Override
    protected boolean needIconDrawable() {
        return true;
    }

    @Override
    protected boolean isNeedShowIcon() {
        return needShowIcon;
    }

    @Override
    protected Drawable getDrawable() {
        if (!TextUtils.equals(getFavListType(), Item.LIST_EDIT_ALL_FAV_LIST)) {
            return mContext.getDrawable(R.drawable.ic_star_white);
        } else {
            return mContext.getDrawable(R.drawable.ic_right_white/*R.drawable.edit_fav_icon*/);
        }
    }

    @Override
    protected String getLayoutIdInfo() {
        String result = null;
        JSONArray array = new JSONArray();
        JSONObject obj = new JSONObject();
        try {
            array.put(2);//first element represent total number of sub view
            obj = new JSONObject(addTextView(R.id.title_text, mName));
            array.put(obj);
            if (!TextUtils.equals(getFavListType(), Item.LIST_EDIT_ALL_FAV_LIST)) {
                obj = new JSONObject(addIconTextView(R.id.icon_text, R.drawable.ic_star_white));
            } else {
                obj = new JSONObject(addIconTextView(R.id.icon_text, R.drawable.ic_right_white/*R.drawable.edit_fav_icon*/));
            }
            array.put(obj);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (array != null && array.length() > 2) {
            result = array.toString();
        }
        return result;
    }

    /*public int getFavId() {
        int result = -1;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getInt(ChannelDataManager.KEY_SETTINGS_FAV_INDEX);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getChannelId e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }*/

    /*public boolean isAllFavList() {
        boolean result = false;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getBoolean(ChannelDataManager.KEY_SETTINGS_IS_ALL_FAV_LIST);
            }
        } catch (JSONException e) {
            Log.e(TAG, "isAllFavList e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }*/

    public String getFavListType() {
        String result = null;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_FAV_LIST_TYPE);
            }
        } catch (JSONException e) {
            Log.e(TAG, "isAllFavList e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public void setItemType(int type) {
        try {
            if (mJSONObject != null) {
                mJSONObject.put(ChannelDataManager.KEY_SETTINGS_CHANNEL_ITEM_TYPE, type);
            }
        } catch (JSONException e) {
            Log.e(TAG, "setItemType e = " + e.getMessage());
            e.printStackTrace();
        }
    }

    public int getItemType() {
        int result = ACTION_CHANNEL_SORT_ALL;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getInt(ChannelDataManager.KEY_SETTINGS_CHANNEL_ITEM_TYPE);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getItemType e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public void setContainerType(int type) {
        try {
            if (mJSONObject != null) {
                mJSONObject.put(ChannelDataManager.KEY_SETTINGS_CHANNEL_CONTAINER_TYPE, type);
            }
        } catch (JSONException e) {
            Log.e(TAG, "setContainerType e = " + e.getMessage());
            e.printStackTrace();
        }
    }

    public int getContainerType() {
        int result = CONTAINER_ITEM_ALL_CHANNEL;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getInt(ChannelDataManager.KEY_SETTINGS_CHANNEL_CONTAINER_TYPE);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getContainerType e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }
}
