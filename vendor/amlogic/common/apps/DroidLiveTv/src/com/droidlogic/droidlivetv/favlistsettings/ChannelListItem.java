package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import com.droidlogic.droidlivetv.R;

public class ChannelListItem extends Item {
    private static final String TAG = ChannelListItem.class.getSimpleName();

    private Context mContext;
    private String mName;
    private boolean mNeedShowIcon = false;
    private String mInfoJson;
    private JSONObject mJSONObject;

    public ChannelListItem(Context context, String name, boolean needShow, String infoJson) {
        super(ChannelListItem.class.getSimpleName());
        this.mContext = context;
        this.mName = name;
        this.mNeedShowIcon = needShow;
        this.mInfoJson = infoJson;
        this.initJSONObject(infoJson);
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

    @Override
    protected JSONObject getJSONObject() {
        return mJSONObject;
    }

    @Override
    protected int getResourceId() {
        return R.layout.item_channellist_layout;
    }

    @Override
    protected String getTitle() {
        return mName;
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

    public String getOriginDisplayNumber() {
        String result = null;
        try {
            if (mJSONObject != null) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_NUMBER);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return result;
    }

    public void setTitle(String value) {
        mName = value;
        /*if (mJSONObject != null) {
            try {
                mJSONObject.put(ChannelDataManager.KEY_SETTINGS_CHANNEL_NAME, value);
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }*/
    }

    public void setJSONObject(JSONObject obj) {
        mJSONObject = obj;
    }

    @Override
    protected boolean isNeedShowIcon() {
        return mNeedShowIcon;
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
    protected Drawable getDrawable() {
        return mContext.getDrawable(R.drawable.ic_star_white);
    }

    @Override
    protected String getLayoutIdInfo() {
        String result = null;
        JSONArray array = new JSONArray();
        JSONObject obj = null;
        try {
            array.put(2);//first element represent total number of sub view
            obj = new JSONObject(addTextView(R.id.title_text, mName));
            array.put(obj);
            obj = new JSONObject(addIconTextView(R.id.icon_text, R.drawable.ic_star_white));
            array.put(obj);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (array != null && array.length() > 2) {
            result = array.toString();
        }
        return result;
    }

    public List<String> getFavAllIndex() {
        List<String> result = new ArrayList<String>();
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                String arrayString = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_FAV_INDEX);
                JSONArray array = new JSONArray(arrayString);
                if (array != null && array.length() > 0) {
                    for (int i = 0; i < array.length(); i++) {
                        result.add(array.getString(i));
                    }
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "getFavAllIndex e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public String getUpdateFavAllIndexArrayString(boolean isSelected, String favTitle) {
        String result = null;
        JSONArray array = new JSONArray();
        List<String> list = getFavAllIndex();
        boolean hasDealt = false;
        if (list != null && list.size() > 0) {
            Iterator<String> iterator = list.iterator();
            while (iterator.hasNext()) {
                String title = (String)iterator.next();
                if (TextUtils.equals(favTitle, title)) {
                    if (!isSelected) {
                        continue;
                    }
                    hasDealt = true;
                }
                array.put(title);
            }
        }
        if (!hasDealt && isSelected) {
            array.put(favTitle);
        }
        if (array != null && array.length() > 0) {
            mNeedShowIcon = true;
        } else {
            mNeedShowIcon = false;
        }
        result = array.toString();
        try {
            if (mJSONObject != null) {
                mJSONObject.put(ChannelDataManager.KEY_SETTINGS_CHANNEL_FAV_INDEX, result);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return result;
    }

    public String getFavArrayJsonStr() {
        String result = null;
        try {
            if (mJSONObject != null) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_FAV_INDEX);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return result;
    }

    public long getChannelId() {
        long result = -1;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getLong(ChannelDataManager.KEY_SETTINGS_CHANNEL_ID);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getChannelId e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public String getChannelType() {
        String result = null;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_TYPE);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getChannelType e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public String getTranponder() {
        String result = null;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_TRANSPONDER);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getTranponder e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public String getSatellite() {
        String result = null;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_SATELLITE);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getSatellite e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public int getFrequency() {
        int result = -1;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getInt(ChannelDataManager.KEY_SETTINGS_CHANNEL_FREQUENCY);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getFrequency e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }

    public int getNetworkId() {
        int result = -1;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getInt(ChannelDataManager.KEY_SETTINGS_CHANNEL_NETWORK_ID);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getNetworkId e = " + e.getMessage());
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

    public String getKey() {
        String result = null;
        try {
            if (mJSONObject != null && mJSONObject.length() > 0) {
                result = mJSONObject.getString(ChannelDataManager.KEY_SETTINGS_CHANNEL_ITEM_KEY);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getContainerType e = " + e.getMessage());
            e.printStackTrace();
        }
        return result;
    }
}
