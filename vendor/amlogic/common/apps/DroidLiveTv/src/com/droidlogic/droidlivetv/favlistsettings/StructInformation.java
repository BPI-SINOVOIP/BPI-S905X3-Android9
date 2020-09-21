package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class StructInformation {
    private static final String TAG = "StructInformation";

    public class ChannelItem {
        private final String KEY_CHANNEL_LIST = "channel_list";
        private final String KEY_NAME = "channel_name";
        private final String FAV_INFO = "fav_infomation";
        private String mExtraJson;
        private Context mContext;

        public ChannelItem(Context context, String json) {
            this.mContext = context;
            this.mExtraJson = json;
        }

        public String getName() {
            String result = null;
            Object obj = getKeyValueStringFromJson(mExtraJson, KEY_NAME);
            if (obj != null && obj instanceof String) {
                result = (String)obj;
            }
            return result;
        }

        public boolean isFavourite() {
            boolean result = false;
            List<Integer> list = getFavInfo();
            if (list != null && list.size() > 0) {
                if (list.get(0) >= 0 && list.get(0) < 64) {
                    result = true;
                }
            }
            return result;
        }

        public List<Integer> getFavInfo() {
            List<Integer> result = new ArrayList<Integer>();
            Object obj = getKeyValueStringFromJson(mExtraJson, FAV_INFO);
            if (obj != null && obj instanceof JSONArray) {
                JSONArray array = (JSONArray)obj;
                if (array != null && array.length() > 0) {
                    for (int i = 0; i < array.length(); i++) {
                        try {
                            result.add((Integer)array.get(i));
                        } catch (JSONException e) {
                            Log.d(TAG, "getFavInfo JSONException " + e);
                            e.printStackTrace();
                        }
                    }
                }
            }
            return result;
        }

        public Object getExtraObject(String key) {
            return getKeyValueStringFromJson(mExtraJson, key);
        }
    }

    public class FavListItem {
        private final String KEY_FAV_LIST = "fav_list";
        private final String KEY_NAME = "fav_name";
        private final String FAV_INFO = "fav_infomation";
        private String mName;
        private boolean mIsFav;
        private String mExtraJson;
        private Context mContext;

        public FavListItem(Context context, String json) {
            this.mContext = context;
            this.mExtraJson = json;
        }

        public List<Integer> getFavInfoList() {
            List<Integer> result = new ArrayList<Integer>();
            Object obj = getKeyValueStringFromJson(mExtraJson, KEY_FAV_LIST);
            if (obj != null && obj instanceof JSONArray) {
                JSONArray array = (JSONArray)obj;
                if (array != null && array.length() > 0) {
                    for (int i = 0; i < array.length(); i++) {
                        result.add(i);
                    }
                }
            }
            return result;
        }

        public Object getExtraObject(String key) {
            return getKeyValueStringFromJson(mExtraJson, key);
        }
    }

    public static JSONObject getJSONObjectFromJson(String json) {
        JSONObject obj = null;
        if (!TextUtils.isEmpty(json)) {
            try {
                obj = new JSONObject(json);
            } catch (JSONException e) {
                Log.d(TAG, "getJSONObjectFromJson JSONException " + e);
                e.printStackTrace();
            }
        }
        return obj;
    }

    public static Object getKeyValueStringFromJson(String json, String key) {
        Object result = null;
        JSONObject obj = getJSONObjectFromJson(json);
        if (obj != null && obj.length() > 0 && !TextUtils.isEmpty(key)) {
            try {
                result = obj.get(key);
            } catch (JSONException e) {
                Log.d(TAG, "getKeyValueFromJson JSONException " + e);
                e.printStackTrace();
            }
        }
        return result;
    }
}
