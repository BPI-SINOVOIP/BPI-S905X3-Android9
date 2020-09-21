/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.systemui.statusbar.notification;

import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.view.NotificationHeaderView;
import android.view.View;

import com.android.systemui.statusbar.CrossFadeHelper;
import com.android.systemui.statusbar.ExpandableNotificationRow;
import com.android.systemui.statusbar.TransformableView;

/**
 * Wraps the actual notification content view; used to implement behaviors which are different for
 * the individual templates and custom views.
 */
public abstract class NotificationViewWrapper implements TransformableView {

    protected final View mView;
    protected final ExpandableNotificationRow mRow;

    private int mBackgroundColor = 0;

    public static NotificationViewWrapper wrap(Context ctx, View v, ExpandableNotificationRow row) {
        if (v.getId() == com.android.internal.R.id.status_bar_latest_event_content) {
            if ("bigPicture".equals(v.getTag())) {
                return new NotificationBigPictureTemplateViewWrapper(ctx, v, row);
            } else if ("bigText".equals(v.getTag())) {
                return new NotificationBigTextTemplateViewWrapper(ctx, v, row);
            } else if ("media".equals(v.getTag()) || "bigMediaNarrow".equals(v.getTag())) {
                return new NotificationMediaTemplateViewWrapper(ctx, v, row);
            } else if ("messaging".equals(v.getTag())) {
                return new NotificationMessagingTemplateViewWrapper(ctx, v, row);
            }
            return new NotificationTemplateViewWrapper(ctx, v, row);
        } else if (v instanceof NotificationHeaderView) {
            return new NotificationHeaderViewWrapper(ctx, v, row);
        } else {
            return new NotificationCustomViewWrapper(ctx, v, row);
        }
    }

    protected NotificationViewWrapper(Context ctx, View view, ExpandableNotificationRow row) {
        mView = view;
        mRow = row;
        onReinflated();
    }

    /**
     * Notifies this wrapper that the content of the view might have changed.
     * @param row the row this wrapper is attached to
     */
    public void onContentUpdated(ExpandableNotificationRow row) {
    }

    public void onReinflated() {
        if (shouldClearBackgroundOnReapply()) {
            mBackgroundColor = 0;
        }
        Drawable background = mView.getBackground();
        if (background instanceof ColorDrawable) {
            mBackgroundColor = ((ColorDrawable) background).getColor();
            mView.setBackground(null);
        }
    }

    protected boolean shouldClearBackgroundOnReapply() {
        return true;
    }

    /**
     * Update the appearance of the expand button.
     *
     * @param expandable should this view be expandable
     * @param onClickListener the listener to invoke when the expand affordance is clicked on
     */
    public void updateExpandability(boolean expandable, View.OnClickListener onClickListener) {}

    /**
     * @return the notification header if it exists
     */
    public NotificationHeaderView getNotificationHeader() {
        return null;
    }

    public int getHeaderTranslation() {
        return 0;
    }

    @Override
    public TransformState getCurrentState(int fadingView) {
        return null;
    }

    @Override
    public void transformTo(TransformableView notification, Runnable endRunnable) {
        // By default we are fading out completely
        CrossFadeHelper.fadeOut(mView, endRunnable);
    }

    @Override
    public void transformTo(TransformableView notification, float transformationAmount) {
        CrossFadeHelper.fadeOut(mView, transformationAmount);
    }

    @Override
    public void transformFrom(TransformableView notification) {
        // By default we are fading in completely
        CrossFadeHelper.fadeIn(mView);
    }

    @Override
    public void transformFrom(TransformableView notification, float transformationAmount) {
        CrossFadeHelper.fadeIn(mView, transformationAmount);
    }

    @Override
    public void setVisible(boolean visible) {
        mView.animate().cancel();
        mView.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
    }

    public int getCustomBackgroundColor() {
        // Parent notifications should always use the normal background color
        return mRow.isSummaryWithChildren() ? 0 : mBackgroundColor;
    }

    protected int resolveBackgroundColor() {
        int customBackgroundColor = getCustomBackgroundColor();
        if (customBackgroundColor != 0) {
            return customBackgroundColor;
        }
        return mView.getContext().getColor(
                com.android.internal.R.color.notification_material_background_color);
    }

    public void setLegacy(boolean legacy) {
    }

    public void setContentHeight(int contentHeight, int minHeightHint) {
    }

    public void setRemoteInputVisible(boolean visible) {
    }

    public void setIsChildInGroup(boolean isChildInGroup) {
    }

    public boolean isDimmable() {
        return true;
    }

    public boolean disallowSingleClick(float x, float y) {
        return false;
    }

    public int getMinLayoutHeight() {
        return 0;
    }

    public boolean shouldClipToRounding(boolean topRounded, boolean bottomRounded) {
        return false;
    }

    public void setHeaderVisibleAmount(float headerVisibleAmount) {
    }

    /**
     * Get the extra height that needs to be added to this view, such that it can be measured
     * normally.
     */
    public int getExtraMeasureHeight() {
        return 0;
    }
}
