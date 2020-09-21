/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.app.Fragment;
import android.os.Bundle;
import android.content.Context;
import android.content.res.XmlResourceParser;
import android.content.res.TypedArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.util.Xml;
import android.widget.ListView;
import android.widget.TextView;
import android.os.SystemProperties;
import java.lang.reflect.Field;

//import com.android.internal.util.XmlUtils;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import com.droidlogic.app.tv.TvControlManager;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.lang.reflect.Method;

import com.droidlogic.tvinput.R;

public class ContentFragment extends Fragment {
    private static final String TAG = "ContentFragment";

    interface XmlReaderListener {
        void handleRequestedNode(Context context, XmlResourceParser parser, AttributeSet attrs)
                throws org.xmlpull.v1.XmlPullParserException, IOException;
    }

    static class XmlReader {
        private final Context mContext;
        private final int mXmlResource;
        private final String mRootNodeName;
        private final String mNodeNameRequested;
        private final XmlReaderListener mListener;

        XmlReader(Context context, int xmlResource, String rootNodeName, String nodeNameRequested,
                  XmlReaderListener listener) {
            mContext = context;
            mXmlResource = xmlResource;
            mRootNodeName = rootNodeName;
            mNodeNameRequested = nodeNameRequested;
            mListener = listener;
        }

        private void skipCurrentTag(XmlResourceParser parser) {
            try {
                Class<?> cls = Class.forName("com.android.internal.util.XmlUtils");
                Object xmlutils = cls.newInstance();
                Method method = cls.getMethod("skipCurrentTag", XmlResourceParser.class);
                method.invoke(parser);
            } catch(Exception e) {
                e.printStackTrace();
            }
        }

        void read() {
            XmlResourceParser parser = null;
            try {
                parser = mContext.getResources().getXml(mXmlResource);
                AttributeSet attrs = Xml.asAttributeSet(parser);


                int type;
                while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                        && type != XmlPullParser.START_TAG) {
                    // Parse next until start tag is found
                }

                String nodeName = parser.getName();
                if (!mRootNodeName.equals(nodeName)) {
                    throw new RuntimeException("XML document must start with <" + mRootNodeName
                            + "> tag; found" + nodeName + " at " + parser.getPositionDescription());
                }

                Bundle curBundle = null;

                final int outerDepth = parser.getDepth();
                while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                        && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                    if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                        continue;
                    }

                    nodeName = parser.getName();
                    if (mNodeNameRequested.equals(nodeName)) {
                        mListener.handleRequestedNode(mContext, parser, attrs);
                    } else {
                        skipCurrentTag(parser);
                    }
                }

            } catch (XmlPullParserException e) {
                throw new RuntimeException("Error parsing headers", e);
            } catch (IOException e) {
                throw new RuntimeException("Error parsing headers", e);
            } finally {
                if (parser != null)
                    parser.close();
            }
        }
    }

    public static final String ITEM_KEY = "item_key";
    public static final String ITEM_NAME = "item_name";
    public static final String ITEM_STATUS = "item_status";

    private Context mContext;
    private View relatedButton;
    private int mContentList;
    ArrayList<HashMap<String, Object>> listItem = null;
    private TextView content_title = null;
    private ContentListView content_list = null;
    ContentAdapter mContentAdapter;

    ContentFragment(int xmlList, View view) {
        mContentList = xmlList;
        listItem = new ArrayList<HashMap<String,Object>>();
        relatedButton = view;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mContext = getActivity();

        View view = (View)inflater.inflate(R.layout.layout_content, container, false);
        content_title = (TextView)view.findViewById(R.id.content_title);
        content_list = (ContentListView)view.findViewById(R.id.content_list);
        content_list.setContentFocus(relatedButton);
        return view;
    }

    @Override
    public void onResume () {
        super.onResume();

        content_list.setAdapter(null);
        listItem.clear();
        new XmlReader(mContext, mContentList, "PreferenceScreen", "Preference", new PreferenceXmlReaderListener()).read();
        mContentAdapter = new ContentAdapter(mContext, listItem);
        content_list.setAdapter(mContentAdapter);
    }

    private class PreferenceXmlReaderListener implements XmlReaderListener {
        PreferenceXmlReaderListener() {
        }

        @Override
        public void handleRequestedNode(Context context, XmlResourceParser parser,
                AttributeSet attrs) throws XmlPullParserException, IOException {

            TypedArray sa = null;//context.getResources().obtainAttributes(attrs,
                    //com.android.internal.R.styleable.Preference);

            String key = null;//getStringFromTypedArray(sa,
                    //com.android.internal.R.styleable.Preference_key);
            String title = null; //getStringFromTypedArray(sa,
                    //com.android.internal.R.styleable.Preference_title);
            sa.recycle();

            //Log.d(TAG, "@@@@@@@@@@@@@@@@ key=" + key + "  title=" + title);
            if (key.equals(SettingsManager.KEY_PICTURE) ||
                key.equals(SettingsManager.KEY_SOUND) ||
                key.equals(SettingsManager.KEY_CHANNEL) ||
                key.equals(SettingsManager.KEY_SETTINGS)) {
                getSettingsManager().setTag(key);
                content_title.setText(title);
            } else {
                if (key.equals(SettingsManager.KEY_TINT) && !getSettingsManager().isShowTint())
                    return;
                if (getSettingsManager().getCurentTvSource() != TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV
                        && (key.equals(SettingsManager.KEY_DEFAULT_LANGUAGE)
                                || key.equals(SettingsManager.KEY_SUBTITLE_SWITCH)
                                || key.equals(SettingsManager.KEY_AD_SWITCH)
                                || key.equals(SettingsManager.KEY_AD_MIX)))
                    return;
                if (getSettingsManager().getCurentTvSource() != TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI &&
                        key.equals(SettingsManager.KEY_HDMI20))
                    return;
                if (key.equals(SettingsManager.KEY_DTV_TYPE) &&
                        (getSettingsManager().getCurentTvSource() != TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV
                            && getSettingsManager().getCurentVirtualTvSource() != TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV))
                    return;

                HashMap<String, Object> map = new HashMap<String, Object>();
                map.put(ITEM_KEY, key);
                map.put(ITEM_NAME, title);
                map.put(ITEM_STATUS, getSettingsManager().getStatus(key));
                if (!(key.equals(SettingsManager.KEY_COLOR_TEMPERATURE) && !SystemProperties.getBoolean("ro.product.pq.colortemperature", true))) {
                    listItem.add(map);
                }
            }
        }
    }

    private String getStringFromTypedArray(TypedArray sa, int resourceId) {
        String value = null;
        TypedValue tv = sa.peekValue(resourceId);
        if (tv != null && tv.type == TypedValue.TYPE_STRING) {
            if (tv.resourceId != 0) {
                value = mContext.getString(tv.resourceId);
            } else {
                value = tv.string.toString();
            }
        }
        return value;
    }

    public ArrayList<HashMap<String, Object>> getContentList () {
        return listItem;
    }

    public void refreshList () {
        ArrayList<HashMap<String, Object>> newList = new ArrayList<HashMap<String, Object>>();
        for (int i = 0; i < listItem.size(); i++) {
            HashMap<String, Object> map = new HashMap<String, Object>();
            String key = listItem.get(i).get(ITEM_KEY).toString();
            map.put(ITEM_KEY, key);
            map.put(ITEM_NAME, listItem.get(i).get(ITEM_NAME).toString());
            map.put(ITEM_STATUS, getSettingsManager().getStatus(key));

            newList.add(map);
        }
        listItem.clear();
        listItem.addAll(newList);
        mContentAdapter.notifyDataSetChanged();
    }

    private SettingsManager getSettingsManager() {
        return ((TvSettingsActivity)mContext).getSettingsManager();
    }
}
