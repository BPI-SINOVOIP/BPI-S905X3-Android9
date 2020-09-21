/*
 * Copyright (C) 2015 The Android Open Source Project
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

package androidx.core.content;

import android.content.SharedPreferences;

import androidx.annotation.NonNull;

/**
 * @deprecated This compatibility class is no longer required. Use {@link SharedPreferences}
 * directly.
 */
@Deprecated
public final class SharedPreferencesCompat {

    /**
     * @deprecated This compatibility class is no longer required. Use
     * {@link SharedPreferences.Editor} directly.
     */
    @Deprecated
    public final static class EditorCompat {

        private static EditorCompat sInstance;

        private static class Helper {
            Helper() {
            }

            public void apply(@NonNull SharedPreferences.Editor editor) {
                try {
                    editor.apply();
                } catch (AbstractMethodError unused) {
                    // The app injected its own pre-Gingerbread
                    // SharedPreferences.Editor implementation without
                    // an apply method.
                    editor.commit();
                }
            }
        }

        private final Helper mHelper;

        private EditorCompat() {
            mHelper = new Helper();
        }
        /**
         * @deprecated This compatibility class is no longer required. Use
         * {@link SharedPreferences.Editor} directly.
         */
        @Deprecated
        public static EditorCompat getInstance() {
            if (sInstance == null) {
                sInstance = new EditorCompat();
            }
            return sInstance;
        }
        /**
         * @deprecated This compatibility method is no longer required. Use
         * {@link SharedPreferences.Editor#apply()} directly.
         */
        @Deprecated
        public void apply(@NonNull SharedPreferences.Editor editor) {
            // Note that this redirection is needed to not break the public API chain
            // of getInstance().apply() calls. Otherwise this method could (and should)
            // be static.
            mHelper.apply(editor);
        }
    }

    private SharedPreferencesCompat() {}

}
