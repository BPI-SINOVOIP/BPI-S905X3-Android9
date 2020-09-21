/*
* Copyright (C) 2013 Samsung System LSI
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
package com.android.bluetooth.map;

import android.util.Log;
import android.util.Xml;

import com.android.bluetooth.DeviceWorkArounds;
import com.android.internal.util.FastXmlSerializer;

import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class BluetoothMapMessageListing {
    private boolean mHasUnread = false;
    private static final String TAG = "BluetoothMapMessageListing";
    private static final boolean D = BluetoothMapService.DEBUG;

    private List<BluetoothMapMessageListingElement> mList;

    public BluetoothMapMessageListing() {
        mList = new ArrayList<BluetoothMapMessageListingElement>();
    }

    public void add(BluetoothMapMessageListingElement element) {
        mList.add(element);
        /* update info regarding whether the list contains unread messages */
        if (!element.getReadBool()) {
            mHasUnread = true;
        }
    }

    /**
     * Used to fetch the number of BluetoothMapMessageListingElement elements in the list.
     * @return the number of elements in the list.
     */
    public int getCount() {
        if (mList != null) {
            return mList.size();
        }
        return 0;
    }

    /**
     * does the list contain any unread messages
     * @return true if unread messages have been added to the list, else false
     */
    public boolean hasUnread() {
        return mHasUnread;
    }


    /**
     *  returns the entire list as a list
     * @return list
     */
    public List<BluetoothMapMessageListingElement> getList() {
        return mList;
    }

    /**
     * Encode the list of BluetoothMapMessageListingElement(s) into a UTF-8
     * formatted XML-string in a trimmed byte array
     *
     * @param version the version as a string.
     *        Set the listing version to e.g. "1.0" or "1.1".
     *        To make this future proof, no check is added to validate the value, hence be careful.
     * @return a reference to the encoded byte array.
     * @throws UnsupportedEncodingException
     *             if UTF-8 encoding is unsupported on the platform.
     */
    // TODO: Remove includeThreadId when MAP-IM is adopted
    public byte[] encode(boolean includeThreadId, String version)
            throws UnsupportedEncodingException {
        StringWriter sw = new StringWriter();
        XmlSerializer xmlMsgElement = null;
        boolean isBenzCarkit = DeviceWorkArounds.addressStartsWith(
                BluetoothMapService.getRemoteDevice().getAddress(),
                DeviceWorkArounds.MERCEDES_BENZ_CARKIT);
        try {
            if (isBenzCarkit) {
                Log.d(TAG, "java_interop: Remote is Mercedes Benz, "
                        + "using Xml Workaround.");
                xmlMsgElement = Xml.newSerializer();
                xmlMsgElement.setOutput(sw);
                xmlMsgElement.text("\n");
            } else {
                xmlMsgElement = new FastXmlSerializer();
                xmlMsgElement.setOutput(sw);
                xmlMsgElement.startDocument("UTF-8", true);
                xmlMsgElement.setFeature(
                        "http://xmlpull.org/v1/doc/features.html#indent-output", true);
            }
            xmlMsgElement.startTag(null, "MAP-msg-listing");
            xmlMsgElement.attribute(null, "version", version);
            // Do the XML encoding of list
            for (BluetoothMapMessageListingElement element : mList) {
                element.encode(xmlMsgElement, includeThreadId); // Append the list element
            }
            xmlMsgElement.endTag(null, "MAP-msg-listing");
            xmlMsgElement.endDocument();
        } catch (IllegalArgumentException e) {
            Log.w(TAG, e);
        } catch (IllegalStateException e) {
            Log.w(TAG, e);
        } catch (IOException e) {
            Log.w(TAG, e);
        }
        /* Fix IOT issue to replace '&amp;' by '&', &lt; by < and '&gt; by '>' in MessageListing */
        if (DeviceWorkArounds.addressStartsWith(BluetoothMapService.getRemoteDevice().getAddress(),
                    DeviceWorkArounds.BREZZA_ZDI_CARKIT)) {
            return sw.toString()
                    .replaceAll("&amp;", "&")
                    .replaceAll("&lt;", "<")
                    .replaceAll("&gt;", ">")
                    .getBytes("UTF-8");
        }
        return sw.toString().getBytes("UTF-8");
    }

    public void sort() {
        Collections.sort(mList);
    }

    public void segment(int count, int offset) {
        count = Math.min(count, mList.size() - offset);
        if (count > 0) {
            mList = mList.subList(offset, offset + count);
            if (mList == null) {
                mList = new ArrayList<BluetoothMapMessageListingElement>(); // Return an empty list
            }
        } else {
            if (offset > mList.size()) {
                mList = new ArrayList<BluetoothMapMessageListingElement>();
                Log.d(TAG, "offset greater than list size. Returning empty list");
            } else {
                mList = mList.subList(offset, mList.size());
            }
        }
    }
}
