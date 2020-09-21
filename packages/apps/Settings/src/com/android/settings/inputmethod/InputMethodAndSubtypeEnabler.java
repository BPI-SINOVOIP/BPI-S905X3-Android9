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
 * limitations under the License.
 */

package com.android.settings.inputmethod;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settingslib.inputmethod.InputMethodAndSubtypeEnablerManager;

public class InputMethodAndSubtypeEnabler extends SettingsPreferenceFragment {
    private InputMethodAndSubtypeEnablerManager mManager;

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.INPUTMETHOD_SUBTYPE_ENABLER;
    }

    @Override
    public void onCreate(final Bundle icicle) {
        super.onCreate(icicle);

        // Input method id should be available from an Intent when this preference is launched as a
        // single Activity (see InputMethodAndSubtypeEnablerActivity). It should be available
        // from a preference argument when the preference is launched as a part of the other
        // Activity (like a right pane of 2-pane Settings app)
        final String targetImi = getStringExtraFromIntentOrArguments(
                android.provider.Settings.EXTRA_INPUT_METHOD_ID);

        final PreferenceScreen root =
                getPreferenceManager().createPreferenceScreen(getPrefContext());
        mManager = new InputMethodAndSubtypeEnablerManager(this);
        mManager.init(this, targetImi, root);
        setPreferenceScreen(root);
    }

    private String getStringExtraFromIntentOrArguments(final String name) {
        final Intent intent = getActivity().getIntent();
        final String fromIntent = intent.getStringExtra(name);
        if (fromIntent != null) {
            return fromIntent;
        }
        final Bundle arguments = getArguments();
        return (arguments == null) ? null : arguments.getString(name);
    }

    @Override
    public void onActivityCreated(final Bundle icicle) {
        super.onActivityCreated(icicle);
        final String title = getStringExtraFromIntentOrArguments(Intent.EXTRA_TITLE);
        if (!TextUtils.isEmpty(title)) {
            getActivity().setTitle(title);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mManager.refresh(getContext(), this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mManager.save(getContext(), this);
    }
}
