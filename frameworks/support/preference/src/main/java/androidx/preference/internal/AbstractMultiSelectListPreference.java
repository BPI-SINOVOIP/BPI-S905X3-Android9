/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.preference.internal;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.RestrictTo;
import androidx.preference.DialogPreference;

import java.util.Set;

/**
 * Stub superclass for {@link androidx.preference.MultiSelectListPreference} so that we
 * can reference it from
 * {@link androidx.preference.MultiSelectListPreferenceDialogFragmentCompat}
 *
 * @hide
 */
@RestrictTo(LIBRARY_GROUP)
public abstract class AbstractMultiSelectListPreference extends DialogPreference {
    public AbstractMultiSelectListPreference(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public AbstractMultiSelectListPreference(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public AbstractMultiSelectListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public AbstractMultiSelectListPreference(Context context) {
        super(context);
    }

    public abstract CharSequence[] getEntries();
    public abstract Set<String> getValues();
    public abstract CharSequence[] getEntryValues();
    public abstract void setValues(Set<String> values);
}
