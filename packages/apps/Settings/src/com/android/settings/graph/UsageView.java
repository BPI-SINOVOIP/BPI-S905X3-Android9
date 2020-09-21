/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.settings.graph;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.SparseIntArray;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.android.settingslib.R;

public class UsageView extends FrameLayout {

    private final UsageGraph mUsageGraph;
    private final TextView[] mLabels;
    private final TextView[] mBottomLabels;

    public UsageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        LayoutInflater.from(context).inflate(R.layout.usage_view, this);
        mUsageGraph = findViewById(R.id.usage_graph);
        mLabels = new TextView[] {
                findViewById(R.id.label_bottom),
                findViewById(R.id.label_middle),
                findViewById(R.id.label_top),
        };
        mBottomLabels = new TextView[] {
                findViewById(R.id.label_start),
                findViewById(R.id.label_end),
        };
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.UsageView, 0, 0);
        if (a.hasValue(R.styleable.UsageView_sideLabels)) {
            setSideLabels(a.getTextArray(R.styleable.UsageView_sideLabels));
        }
        if (a.hasValue(R.styleable.UsageView_bottomLabels)) {
            setBottomLabels(a.getTextArray(R.styleable.UsageView_bottomLabels));
        }
        if (a.hasValue(R.styleable.UsageView_textColor)) {
            int color = a.getColor(R.styleable.UsageView_textColor, 0);
            for (TextView v : mLabels) {
                v.setTextColor(color);
            }
            for (TextView v : mBottomLabels) {
                v.setTextColor(color);
            }
        }
        if (a.hasValue(R.styleable.UsageView_android_gravity)) {
            int gravity = a.getInt(R.styleable.UsageView_android_gravity, 0);
            if (gravity == Gravity.END) {
                LinearLayout layout = findViewById(R.id.graph_label_group);
                LinearLayout labels = findViewById(R.id.label_group);
                // Swap the children order.
                layout.removeView(labels);
                layout.addView(labels);
                // Set gravity.
                labels.setGravity(Gravity.END);
                // Swap the bottom space order.
                LinearLayout bottomLabels = findViewById(R.id.bottom_label_group);
                View bottomSpace = bottomLabels.findViewById(R.id.bottom_label_space);
                bottomLabels.removeView(bottomSpace);
                bottomLabels.addView(bottomSpace);
            } else if (gravity != Gravity.START) {
                throw new IllegalArgumentException("Unsupported gravity " + gravity);
            }
        }
        mUsageGraph.setAccentColor(a.getColor(R.styleable.UsageView_android_colorAccent, 0));
        a.recycle();
    }

    public void clearPaths() {
        mUsageGraph.clearPaths();
    }

    public void addPath(SparseIntArray points) {
        mUsageGraph.addPath(points);
    }

    public void addProjectedPath(SparseIntArray points) {
        mUsageGraph.addProjectedPath(points);
    }

    public void configureGraph(int maxX, int maxY) {
        mUsageGraph.setMax(maxX, maxY);
    }

    public void setAccentColor(int color) {
        mUsageGraph.setAccentColor(color);
    }

    public void setDividerLoc(int dividerLoc) {
        mUsageGraph.setDividerLoc(dividerLoc);
    }

    public void setDividerColors(int middleColor, int topColor) {
        mUsageGraph.setDividerColors(middleColor, topColor);
    }

    public void setSideLabelWeights(float before, float after) {
        setWeight(R.id.space1, before);
        setWeight(R.id.space2, after);
    }

    private void setWeight(int id, float weight) {
        View v = findViewById(id);
        LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) v.getLayoutParams();
        params.weight = weight;
        v.setLayoutParams(params);
    }

    public void setSideLabels(CharSequence[] labels) {
        if (labels.length != mLabels.length) {
            throw new IllegalArgumentException("Invalid number of labels");
        }
        for (int i = 0; i < mLabels.length; i++) {
            mLabels[i].setText(labels[i]);
        }
    }

    public void setBottomLabels(CharSequence[] labels) {
        if (labels.length != mBottomLabels.length) {
            throw new IllegalArgumentException("Invalid number of labels");
        }
        for (int i = 0; i < mBottomLabels.length; i++) {
            mBottomLabels[i].setText(labels[i]);
        }
    }

}
