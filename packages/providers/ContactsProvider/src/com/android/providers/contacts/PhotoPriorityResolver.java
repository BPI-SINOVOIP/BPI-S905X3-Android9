/*
 * Copyright (C) 2010 The Android Open Source Project
 *
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
 * limitations under the License
 */

package com.android.providers.contacts;

import android.accounts.AccountManager;
import android.accounts.AuthenticatorDescription;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.res.XmlResourceParser;
import android.util.ArrayMap;

import com.android.internal.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.List;

/**
 * Maintains a cache of photo priority per account type.  During contact aggregation
 * photo with a higher priority is chosen for the the entire contact, barring an
 * explicit override by the user, which is captured as the is_superprimary flag
 * on the photo itself.
 */
public class PhotoPriorityResolver  {
    private static final String TAG = "PhotoPriorityResolver";

    public static final int DEFAULT_PRIORITY = 7;

    private static final String SYNC_META_DATA = "android.content.SyncAdapter";

    /**
     * The metadata name for so-called "contacts.xml".
     *
     * On LMP and later, we also accept the "alternate" name.
     * This is to allow sync adapters to have a contacts.xml without making it visible on older
     * platforms. If you modify this also update the matching list in
     * ContactsCommon/ExternalAccountType.
     */
    private static final String[] METADATA_CONTACTS_NAMES = new String[] {
            "android.provider.ALTERNATE_CONTACTS_STRUCTURE",
            "android.provider.CONTACTS_STRUCTURE"
    };


    /**
     * The XML tag capturing the picture priority. The syntax is:
     * <code>&lt;Picture android:priority="6"/&gt;</code>
     */
    private static final String PICTURE_TAG = "Picture";

    /**
     * Name of the attribute of the Picture tag capturing the priority itself.
     */
    private static final String PRIORITY_ATTR = "priority";

    private Context mContext;
    private ArrayMap<String, Integer> mPhotoPriorities = new ArrayMap<>();

    public PhotoPriorityResolver(Context context) {
        mContext = context;
    }

    /**
     * Returns the photo priority for the specified account type.  Maintains cache
     * of photo priorities.
     */
    public synchronized int getPhotoPriority(String accountType) {
        if (accountType == null) {
            return DEFAULT_PRIORITY;
        }

        Integer priority = mPhotoPriorities.get(accountType);
        if (priority == null) {
            priority = resolvePhotoPriority(accountType);
            mPhotoPriorities.put(accountType, priority);
        }
        return priority;
     }

    /**
     * Finds photo priority for the specified account type.
     */
    private int resolvePhotoPriority(String accountType) {
        final AccountManager am = AccountManager.get(mContext);

        for (AuthenticatorDescription auth : am.getAuthenticatorTypes()) {
            if (accountType.equals(auth.type)) {
                return resolvePhotoPriorityFromMetaData(auth.packageName);
            }
        }

        return DEFAULT_PRIORITY;
    }

    /**
     * Finds the meta-data XML containing the contacts configuration and
     * reads the picture priority from that file.
     */
    /* package */ int resolvePhotoPriorityFromMetaData(String packageName) {
        final PackageManager pm = mContext.getPackageManager();
        final Intent intent = new Intent(SYNC_META_DATA).setPackage(packageName);
        final List<ResolveInfo> intentServices = pm.queryIntentServices(intent,
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);

        if (intentServices != null) {
            for (final ResolveInfo resolveInfo : intentServices) {
                final ServiceInfo serviceInfo = resolveInfo.serviceInfo;
                if (serviceInfo == null) {
                    continue;
                }
                for (String metadataName : METADATA_CONTACTS_NAMES) {
                    final XmlResourceParser parser = serviceInfo.loadXmlMetaData(
                            pm, metadataName);
                    if (parser != null) {
                        return loadPhotoPriorityFromXml(mContext, parser);
                    }
                }
            }
        }
        return DEFAULT_PRIORITY;
    }

    private int loadPhotoPriorityFromXml(Context context, XmlPullParser parser) {
        int priority = DEFAULT_PRIORITY;
        try {
            int type;
            while ((type = parser.next()) != XmlPullParser.START_TAG
                    && type != XmlPullParser.END_DOCUMENT) {
                // Drain comments and whitespace
            }

            if (type != XmlPullParser.START_TAG) {
                throw new IllegalStateException("No start tag found");
            }

            final int depth = parser.getDepth();
            while (((type = parser.next()) != XmlPullParser.END_TAG || parser.getDepth() > depth)
                    && type != XmlPullParser.END_DOCUMENT) {
                String name = parser.getName();
                if (type == XmlPullParser.START_TAG && PICTURE_TAG.equals(name)) {
                    int attributeCount = parser.getAttributeCount();
                    for (int i = 0; i < attributeCount; i++) {
                        String attr = parser.getAttributeName(i);
                        if (PRIORITY_ATTR.equals(attr)) {
                            priority = XmlUtils.convertValueToInt(parser.getAttributeValue(i),
                                    DEFAULT_PRIORITY);
                        } else {
                            throw new IllegalStateException("Unsupported attribute " + attr);
                        }
                    }
                }
            }
        } catch (XmlPullParserException e) {
            throw new IllegalStateException("Problem reading XML", e);
        } catch (IOException e) {
            throw new IllegalStateException("Problem reading XML", e);
        }

        return priority;
    }
}
