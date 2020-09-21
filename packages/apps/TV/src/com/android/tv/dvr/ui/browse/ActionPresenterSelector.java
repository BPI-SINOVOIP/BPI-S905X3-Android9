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
 * limitations under the License
 */

package com.android.tv.dvr.ui.browse;

import android.graphics.drawable.Drawable;
import android.support.v17.leanback.R;
import android.support.v17.leanback.widget.Action;
import android.support.v17.leanback.widget.Presenter;
import android.support.v17.leanback.widget.PresenterSelector;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

// This class is adapted from Leanback's library, which does not support action icon with one-line
// label. This class modified its getPresenter method to support the above situation.
class ActionPresenterSelector extends PresenterSelector {
    private final Presenter mOneLineActionPresenter = new OneLineActionPresenter();
    private final Presenter mTwoLineActionPresenter = new TwoLineActionPresenter();
    private final Presenter[] mPresenters =
            new Presenter[] {mOneLineActionPresenter, mTwoLineActionPresenter};

    @Override
    public Presenter getPresenter(Object item) {
        Action action = (Action) item;
        if (TextUtils.isEmpty(action.getLabel2()) && action.getIcon() == null) {
            return mOneLineActionPresenter;
        } else {
            return mTwoLineActionPresenter;
        }
    }

    @Override
    public Presenter[] getPresenters() {
        return mPresenters;
    }

    static class ActionViewHolder extends Presenter.ViewHolder {
        Action mAction;
        Button mButton;
        int mLayoutDirection;

        public ActionViewHolder(View view, int layoutDirection) {
            super(view);
            mButton = (Button) view.findViewById(R.id.lb_action_button);
            mLayoutDirection = layoutDirection;
        }
    }

    class OneLineActionPresenter extends Presenter {
        @Override
        public ViewHolder onCreateViewHolder(ViewGroup parent) {
            View v =
                    LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.lb_action_1_line, parent, false);
            return new ActionViewHolder(v, parent.getLayoutDirection());
        }

        @Override
        public void onBindViewHolder(Presenter.ViewHolder viewHolder, Object item) {
            Action action = (Action) item;
            ActionViewHolder vh = (ActionViewHolder) viewHolder;
            vh.mAction = action;
            vh.mButton.setText(action.getLabel1());
        }

        @Override
        public void onUnbindViewHolder(Presenter.ViewHolder viewHolder) {
            ((ActionViewHolder) viewHolder).mAction = null;
        }
    }

    class TwoLineActionPresenter extends Presenter {
        @Override
        public ViewHolder onCreateViewHolder(ViewGroup parent) {
            View v =
                    LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.lb_action_2_lines, parent, false);
            return new ActionViewHolder(v, parent.getLayoutDirection());
        }

        @Override
        public void onBindViewHolder(Presenter.ViewHolder viewHolder, Object item) {
            Action action = (Action) item;
            ActionViewHolder vh = (ActionViewHolder) viewHolder;
            Drawable icon = action.getIcon();
            vh.mAction = action;

            if (icon != null) {
                final int startPadding =
                        vh.view
                                .getResources()
                                .getDimensionPixelSize(R.dimen.lb_action_with_icon_padding_start);
                final int endPadding =
                        vh.view
                                .getResources()
                                .getDimensionPixelSize(R.dimen.lb_action_with_icon_padding_end);
                vh.view.setPaddingRelative(startPadding, 0, endPadding, 0);
            } else {
                final int padding =
                        vh.view
                                .getResources()
                                .getDimensionPixelSize(R.dimen.lb_action_padding_horizontal);
                vh.view.setPaddingRelative(padding, 0, padding, 0);
            }
            vh.mButton.setCompoundDrawablesRelativeWithIntrinsicBounds(icon, null, null, null);

            CharSequence line1 = action.getLabel1();
            CharSequence line2 = action.getLabel2();
            if (TextUtils.isEmpty(line1)) {
                vh.mButton.setText(line2);
            } else if (TextUtils.isEmpty(line2)) {
                vh.mButton.setText(line1);
            } else {
                vh.mButton.setText(line1 + "\n" + line2);
            }
        }

        @Override
        public void onUnbindViewHolder(Presenter.ViewHolder viewHolder) {
            ActionViewHolder vh = (ActionViewHolder) viewHolder;
            vh.mButton.setCompoundDrawablesRelativeWithIntrinsicBounds(null, null, null, null);
            vh.view.setPadding(0, 0, 0, 0);
            vh.mAction = null;
        }
    }
}
