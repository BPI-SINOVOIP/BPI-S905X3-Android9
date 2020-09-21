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

package com.android.tv.common.ui.setup;

import static android.content.Context.ACCESSIBILITY_SERVICE;

import android.os.Bundle;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v17.leanback.widget.VerticalGridView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.AccessibilityDelegate;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.LinearLayout;
import com.android.tv.common.R;

/** A fragment for channel source info/setup. */
public abstract class SetupGuidedStepFragment extends GuidedStepFragment {
    /**
     * Key of the argument which indicate whether the parent of this fragment has three panes.
     *
     * <p>Value type: boolean
     */
    public static final String KEY_THREE_PANE = "key_three_pane";

    private View mContentFragment;
    private boolean mFromContentFragment;
    private boolean mAccessibilityMode;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        Bundle arguments = getArguments();
        view.findViewById(android.support.v17.leanback.R.id.action_fragment_root)
                .setPadding(0, 0, 0, 0);
        mContentFragment = view.findViewById(android.support.v17.leanback.R.id.content_fragment);
        LinearLayout.LayoutParams guidanceLayoutParams =
                (LinearLayout.LayoutParams) mContentFragment.getLayoutParams();
        guidanceLayoutParams.weight = 0;
        if (arguments != null && arguments.getBoolean(KEY_THREE_PANE, false)) {
            // Content fragment.
            guidanceLayoutParams.width =
                    getResources()
                            .getDimensionPixelOffset(
                                    R.dimen.setup_guidedstep_guidance_section_width_3pane);
            int doneButtonWidth =
                    getResources()
                            .getDimensionPixelOffset(R.dimen.setup_done_button_container_width);
            // Guided actions list
            View list = view.findViewById(android.support.v17.leanback.R.id.guidedactions_list);
            MarginLayoutParams marginLayoutParams = (MarginLayoutParams) list.getLayoutParams();
            // Use content view to check layout direction while view is being created.
            if (getResources().getConfiguration().getLayoutDirection()
                    == View.LAYOUT_DIRECTION_LTR) {
                marginLayoutParams.rightMargin = doneButtonWidth;
            } else {
                marginLayoutParams.leftMargin = doneButtonWidth;
            }
        } else {
            // Content fragment.
            guidanceLayoutParams.width =
                    getResources()
                            .getDimensionPixelOffset(
                                    R.dimen.setup_guidedstep_guidance_section_width_2pane);
        }
        // gridView Alignment
        VerticalGridView gridView = getGuidedActionsStylist().getActionsGridView();
        int offset =
                getResources()
                        .getDimensionPixelOffset(R.dimen.setup_guidedactions_selector_margin_top);
        gridView.setWindowAlignmentOffset(offset);
        gridView.setWindowAlignmentOffsetPercent(0);
        gridView.setItemAlignmentOffsetPercent(0);
        ((ViewGroup) view.findViewById(android.support.v17.leanback.R.id.guidedactions_list))
                .setTransitionGroup(false);
        // Needed for the shared element transition.
        // content_frame is defined in leanback.
        ViewGroup group =
                (ViewGroup) view.findViewById(android.support.v17.leanback.R.id.content_frame);
        group.setClipChildren(false);
        group.setClipToPadding(false);
        return view;
    }

    @Override
    public GuidedActionsStylist onCreateActionsStylist() {
        return new SetupGuidedStepFragmentGuidedActionsStylist();
    }

    @Override
    public void onResume() {
        super.onResume();
        AccessibilityManager am =
                (AccessibilityManager) getActivity().getSystemService(ACCESSIBILITY_SERVICE);
        mAccessibilityMode = am != null && am.isEnabled() && am.isTouchExplorationEnabled();
        mContentFragment.setFocusable(mAccessibilityMode);
        if (mAccessibilityMode) {
            mContentFragment.setAccessibilityDelegate(
                new AccessibilityDelegate() {
                    @Override
                    public boolean performAccessibilityAction(View host, int action, Bundle args) {
                        if (action == AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS
                                && !getActions().isEmpty()) {
                            // scroll to the top. This makes the first action view on the screen.
                            // Otherwise, the view can be recycled, so accessibility events cannot
                            // be sent later.
                            getGuidedActionsStylist().getActionsGridView().scrollToPosition(0);
                            mFromContentFragment = true;
                        }
                        return super.performAccessibilityAction(host, action, args);
                    }
                });
            mContentFragment.requestFocus();
        }
    }

    @Override
    public GuidanceStylist onCreateGuidanceStylist() {
        return new GuidanceStylist() {
            @Override
            public View onCreateView(
                    LayoutInflater inflater, ViewGroup container, Guidance guidance) {
                View view = super.onCreateView(inflater, container, guidance);
                if (guidance.getIconDrawable() == null) {
                    // Icon view should not take up space when we don't use image.
                    getIconView().setVisibility(View.GONE);
                }
                return view;
            }
        };
    }

    protected abstract String getActionCategory();

    protected View getDoneButton() {
        return getActivity().findViewById(R.id.button_done);
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (!action.isFocusable()) {
            // an unfocusable action may be clicked in accessibility mode when it's accessibility
            // focused
            return;
        }
        SetupActionHelper.onActionClick(this, getActionCategory(), (int) action.getId());
    }

    @Override
    protected void onProvideFragmentTransitions() {
        // Don't use the fragment transition defined in GuidedStepFragment.
    }

    @Override
    public boolean isFocusOutEndAllowed() {
        return true;
    }

    protected void setAccessibilityDelegate(GuidedActionsStylist.ViewHolder vh,
            GuidedAction action) {
        if (!mAccessibilityMode || findActionPositionById(action.getId()) == 0) {
            return;
        }
        vh.itemView.setAccessibilityDelegate(
                new AccessibilityDelegate() {
                    @Override
                    public boolean performAccessibilityAction(View host, int action, Bundle args) {
                        if ((action == AccessibilityNodeInfo.ACTION_FOCUS
                                || action == AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)
                                && mFromContentFragment) {
                            // block the action and make the first action view accessibility focused
                            View view = getActionItemView(0);
                            if (view != null) {
                                view.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
                                mFromContentFragment = false;
                                return true;
                            }
                        }
                        return super.performAccessibilityAction(host, action, args);
                    }
                });
    }

    private class SetupGuidedStepFragmentGuidedActionsStylist extends GuidedActionsStylist {

        @Override
        public void onBindViewHolder(GuidedActionsStylist.ViewHolder vh, GuidedAction action) {
            super.onBindViewHolder(vh, action);
            setAccessibilityDelegate(vh, action);
        }
    }
}
