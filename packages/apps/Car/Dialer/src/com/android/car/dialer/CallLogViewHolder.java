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
 * limitations under the License.
 */
package com.android.car.dialer;

import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * A {@link android.support.v7.widget.RecyclerView.ViewHolder} that will hold layouts that
 * are inflated by {@link StrequentsAdapter}.
 */
public class CallLogViewHolder extends RecyclerView.ViewHolder {
    public final FrameLayout iconContainer;
    public final ImageView icon;
    public final TextView title;
    public final TextView text;
    public final View card;
    public final ViewGroup container;
    public final LinearLayout callType;
    public final CallTypeIconsView callTypeIconsView;
    public final ImageView smallIcon;

    public CallLogViewHolder(View v) {
        super(v);

        icon = v.findViewById(R.id.icon);
        iconContainer = v.findViewById(R.id.icon_container);
        title = v.findViewById(R.id.title);
        text = v.findViewById(R.id.text);
        card = v.findViewById(R.id.call_log_card);
        callType = v.findViewById(R.id.call_type);
        callTypeIconsView = v.findViewById(R.id.call_type_icons);
        smallIcon = v.findViewById(R.id.small_icon);
        container = v.findViewById(R.id.container);
    }
}
