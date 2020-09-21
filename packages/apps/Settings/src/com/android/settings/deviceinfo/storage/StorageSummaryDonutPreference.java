/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.settings.deviceinfo.storage;

import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.storage.StorageManager;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceViewHolder;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import android.util.AttributeSet;
import android.util.MathUtils;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.widget.DonutView;

import java.text.NumberFormat;
import java.util.Locale;

/**
 * StorageSummaryDonutPreference is a preference which summarizes the used and remaining storage left
 * on a given storage volume. It is visualized with a donut graphing the % used.
 */
public class StorageSummaryDonutPreference extends Preference implements View.OnClickListener {
    private double mPercent = -1;

    public StorageSummaryDonutPreference(Context context) {
        this(context, null);
    }

    public StorageSummaryDonutPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        setLayoutResource(R.layout.storage_summary_donut);
        setEnabled(false);
    }

    public void setPercent(long usedBytes, long totalBytes) {
        if (totalBytes == 0) {
            return;
        }

        mPercent = usedBytes / (double) totalBytes;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder view) {
        super.onBindViewHolder(view);
        view.itemView.setClickable(false);

        final DonutView donut = (DonutView) view.findViewById(R.id.donut);
        if (donut != null) {
            donut.setPercentage(mPercent);
        }

        final Button deletionHelperButton = (Button) view.findViewById(R.id.deletion_helper_button);
        if (deletionHelperButton != null) {
            deletionHelperButton.setOnClickListener(this);
        }
    }

    @Override
    public void onClick(View v) {
        if (v != null && R.id.deletion_helper_button == v.getId()) {
            Context context = getContext();
            FeatureFactory.getFactory(context).getMetricsFeatureProvider().action(
                    context, MetricsEvent.STORAGE_FREE_UP_SPACE_NOW);
            Intent intent = new Intent(StorageManager.ACTION_MANAGE_STORAGE);
            getContext().startActivity(intent);
        }
    }

    private static class BoldLinkSpan extends StyleSpan {
        public BoldLinkSpan() {
            super(Typeface.BOLD);
        }

        @Override
        public void updateDrawState(TextPaint ds) {
            super.updateDrawState(ds);
            ds.setColor(ds.linkColor);
        }
    }
}
