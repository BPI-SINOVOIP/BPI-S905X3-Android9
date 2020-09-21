package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.graphics.drawable.Drawable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.droidlogic.droidlivetv.R;

public class SortListItem extends Item {
    private static final String TAG = "SortListItem";

    private Context mContext;
    private String mName;

    public SortListItem(Context context, String name) {
        super(SortListItem.class.getSimpleName());
        this.mContext = context;
        this.mName = name;
    }

    @Override
    protected JSONObject getJSONObject() {
        return null;
    }

    public String getOriginTitle() {
        return mName;
    }

    @Override
    protected int getResourceId() {
        return R.layout.item_sortlist_layout;
    }

    @Override
    protected String getTitle() {
        return mName;
    }

    protected boolean needTitle() {
        return true;
    }

    protected boolean needIconDrawable() {
        return false;
    }

    @Override
    protected boolean isNeedShowIcon() {
        return false;
    }

    @Override
    protected Drawable getDrawable() {
        return mContext.getDrawable(R.drawable.ic_star_white);
    }

    @Override
    protected String getLayoutIdInfo() {
        String result = null;
        JSONArray array = new JSONArray();
        JSONObject obj = new JSONObject();
        try {
            array.put(1);//first element represent total number of sub view
            obj = new JSONObject(addTextView(R.id.title_text, mName));
            array.put(obj);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (array != null && array.length() > 1) {
            result = array.toString();
        }
        return result;
    }
}
