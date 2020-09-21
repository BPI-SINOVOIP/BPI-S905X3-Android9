package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.LinkedList;

public class ItemAdapter extends CommonAdapter<Item> {
    private static final String TAG = "ItemAdapter";

    private LinkedList<Item> mData;
    private Context mContext;
    private String mTitle;

    public ItemAdapter(LinkedList<Item> data, Context context, String title) {
        super(data, context, title);
        this.mData = data;
        this.mContext = context;
        this.mTitle = title;
    }

    public void setDataByPosition(int position, Item item) {
        super.setDataByPosition(position, item);
        if (mData != null && mData.size() >= position) {
            mData.set(position,item );
        }
    }

    public void setAllData(LinkedList<Item> data) {
        super.setAllData(data);
        mData = data;
    }

    public LinkedList<Item> getAllData() {
        return mData;
    }


    @Override
    public int getCount() {
        return mData.size();
    }

    @Override
    public Item getItem(int position) {
        return mData.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public CommonViewHolder setViewContent(Context context, String itemname, View view, ViewGroup parent, int position, Item item) {
        CommonViewHolder viewHolder = CommonViewHolder.get(context, view, parent, item.getResourceId(), position);
        int titleTextResId = item.getTitleId();
        int iconTextResId = item.getIconTextId();
        String title = item.getTitle();
        Drawable iconDrawable = item.getDrawable();
        //Log.d(TAG, "setViewContent fixed itemname = " +  itemname + ", titleTextResId = " + titleTextResId + ", iconTextResId = " + iconTextResId + ", item = " + item + ", iconDrawable = " + iconDrawable);
        //add extend intreface
        try {
            JSONArray array = new JSONArray(item.getLayoutIdInfo());
            if (array != null && array.length() > 1) {
                int viewNum = (int)array.get(0);
                if (viewNum >= 1) {
                    titleTextResId = ((JSONObject)array.get(1)).getInt("resId");
                    title = ((JSONObject)array.get(1)).getString("value");
                    //Log.d(TAG, "setViewContent parsed titleTextResId = " + titleTextResId + ", title = " + title);
                }
                if (viewNum >= 2) {
                    JSONObject obj = (JSONObject)array.get(2);
                    iconTextResId = obj.getInt("resId");
                    iconDrawable = mContext.getDrawable(obj.getInt("iconRes"));
                    //Log.d(TAG, "setViewContent parsed iconTextResId = " + iconTextResId + ", iconDrawable = " + iconDrawable);
                }
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (item.needTitle()) {
            if (item instanceof ChannelListItem) {
                long channelId = ((ChannelListItem)item).getChannelId();
                if (channelId != -1) {
                    title = /*String.valueOf(position + 1)*/((ChannelListItem)item).getOriginDisplayNumber() + "   " + title;
                }
            }
            viewHolder.setText(titleTextResId, title);
        }
        if (item.needIconDrawable()) {
            //Log.d(TAG, "setViewContent need icon");
            if (item.isNeedShowIcon()) {
                //Log.d(TAG, "setViewContent display icon");
                viewHolder.setIcon(iconTextResId, iconDrawable);
            } else {
                //Log.d(TAG, "setViewContent hide icon");
                viewHolder.setIcon(iconTextResId, null);
            }
        }
        return viewHolder;
    }
}
