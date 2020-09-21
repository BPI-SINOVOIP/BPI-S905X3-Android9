/*
 * Copyright (C) 2009 The Android Open Source Project
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
 * limitations under the License.
 */

package com.droidlogic.inputmethod.remote;

import java.util.Vector;

import android.content.Context;

/**
 * Class used to cache previously loaded soft keyboard layouts.
 */
public class SkbPool {
        private static SkbPool mInstance = null;

        private Vector<SkbTemplate> mSkbTemplates = new Vector<SkbTemplate>();
        private Vector<SoftKeyboard> mSoftKeyboards = new Vector<SoftKeyboard>();

        private SkbPool() {
        }

        public static SkbPool getInstance() {
            if ( null == mInstance ) { mInstance = new SkbPool(); }
            return mInstance;
        }

        public void resetCachedSkb() {
            mSoftKeyboards.clear();
        }

        public SkbTemplate getSkbTemplate ( int skbTemplateId, Context context, boolean force ) {
            for ( int i = 0; i < mSkbTemplates.size() && ( !force ); i++ ) {
                SkbTemplate t = mSkbTemplates.elementAt ( i );
                if ( t.getSkbTemplateId() == skbTemplateId ) {
                    return t;
                }
            }
            if ( null != context || ( force && ( null != context ) ) ) {
                XmlKeyboardLoader xkbl = new XmlKeyboardLoader ( context );
                SkbTemplate t = xkbl.loadSkbTemplate ( skbTemplateId );
                if ( null != t ) {
                    mSkbTemplates.add ( t );
                    return t;
                }
            }
            return null;
        }

        public SoftKeyboard getSoftKeyboard ( int skbCacheId, int skbXmlId,
                                              int skbWidth, int skbHeight, Context context ) {
            return getSoftKeyboard ( skbCacheId,  skbXmlId, skbWidth,  skbHeight,  context, false );
        }
        /*@hide*/
        // Try to find the keyboard in the pool with the cache id. If there is no
        // keyboard found, try to load it with the given xml id.
        public SoftKeyboard getSoftKeyboard ( int skbCacheId, int skbXmlId,
                                              int skbWidth, int skbHeight, Context context, boolean force ) {
            for ( int i = 0; i < mSoftKeyboards.size() && ( !force ); i++ ) {
                SoftKeyboard skb = mSoftKeyboards.elementAt ( i );
                if ( skb.getCacheId() == skbCacheId && skb.getSkbXmlId() == skbXmlId ) {
                    skb.setSkbCoreSize ( skbWidth, skbHeight );
                    skb.setNewlyLoadedFlag ( false );
                    return skb;
                }
            }
            if ( null != context || ( ( null != context ) && force ) ) {
                XmlKeyboardLoader xkbl = new XmlKeyboardLoader ( context );
                SoftKeyboard skb = xkbl.loadKeyboard ( skbXmlId, skbWidth, skbHeight, force );
                if ( skb != null ) {
                    if ( skb.getCacheFlag() ) {
                        skb.setCacheId ( skbCacheId );
                        mSoftKeyboards.add ( skb );
                    }
                }
                return skb;
            }
            return null;
        }
}
