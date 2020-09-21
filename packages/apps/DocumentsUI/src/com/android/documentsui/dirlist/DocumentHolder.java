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

package com.android.documentsui.dirlist;

import android.content.Context;
import android.database.Cursor;
import android.support.v7.widget.RecyclerView;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewPropertyAnimator;
import android.widget.ImageView;

import com.android.documentsui.base.Shared;
import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

import javax.annotation.Nullable;

public abstract class DocumentHolder
        extends RecyclerView.ViewHolder implements View.OnKeyListener {

    static final float DISABLED_ALPHA = 0.3f;

    protected final Context mContext;

    protected @Nullable String mModelId;

    // See #addKeyEventListener for details on the need for this field.
    private KeyboardEventListener mKeyEventListener;

    private final DocumentItemDetails mDetails;

    public DocumentHolder(Context context, ViewGroup parent, int layout) {
        this(context, inflateLayout(context, parent, layout));
    }

    public DocumentHolder(Context context, View item) {
        super(item);

        itemView.setOnKeyListener(this);

        mContext = context;
        mDetails = new DocumentItemDetails();
    }

    /**
     * Binds the view to the given item data.
     * @param cursor
     * @param modelId
     * @param state
     */
    public abstract void bind(Cursor cursor, String modelId);

    public String getModelId() {
        return mModelId;
    }

    /**
     * Makes the associated item view appear selected. Note that this merely affects the appearance
     * of the view, it doesn't actually select the item.
     * TODO: Use the DirectoryItemAnimator instead of manually controlling animation using a boolean
     * flag.
     *
     * @param selected
     * @param animate Whether or not to animate the change. Only selection changes initiated by the
     *            selection manager should be animated. See
     *            {@link ModelBackedDocumentsAdapter#onBindViewHolder(DocumentHolder, int, java.util.List)}
     */
    public void setSelected(boolean selected, boolean animate) {
        itemView.setActivated(selected);
        itemView.setSelected(selected);
    }

    public void setEnabled(boolean enabled) {
        setEnabledRecursive(itemView, enabled);
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        assert(mKeyEventListener != null);
        ItemDetails details = getItemDetails();
        return (details == null)
            ? false
            : mKeyEventListener.onKey(getItemDetails(),  keyCode,  event);
    }

    /**
     * Installs a delegate to receive keyboard input events. This arrangement is necessitated
     * by the fact that a single listener cannot listen to all keyboard events
     * on RecyclerView (our parent view). Not sure why this is, but have been
     * assured it is the case.
     *
     * <p>Ideally we'd not involve DocumentHolder in propagation of events like this.
     */
    public void addKeyEventListener(KeyboardEventListener listener) {
        assert(mKeyEventListener == null);
        mKeyEventListener = listener;
    }

    public boolean inDragRegion(MotionEvent event) {
        return false;
    }

    public boolean inSelectRegion(MotionEvent event) {
        return false;
    }

    public ItemDetails getItemDetails() {
        return mDetails;
    }

    static void setEnabledRecursive(View itemView, boolean enabled) {
        if (itemView == null || itemView.isEnabled() == enabled) {
            return;
        }
        itemView.setEnabled(enabled);

        if (itemView instanceof ViewGroup) {
            final ViewGroup vg = (ViewGroup) itemView;
            for (int i = vg.getChildCount() - 1; i >= 0; i--) {
                setEnabledRecursive(vg.getChildAt(i), enabled);
            }
        }
    }

    @SuppressWarnings("TypeParameterUnusedInFormals")
    private static <V extends View> V inflateLayout(Context context, ViewGroup parent, int layout) {
        final LayoutInflater inflater = LayoutInflater.from(context);
        return (V) inflater.inflate(layout, parent, false);
    }

    static ViewPropertyAnimator fade(ImageView view, float alpha) {
        return view.animate().setDuration(Shared.CHECK_ANIMATION_DURATION).alpha(alpha);
    }

    private final class DocumentItemDetails extends ItemDetails {

        @Override
        public int getPosition() {
            return DocumentHolder.this.getAdapterPosition();
        }

        @Override
        public String getStableId() {
            return DocumentHolder.this.getModelId();
        }

        @Override
        public int getItemViewType() {
            return DocumentHolder.this.getItemViewType();
        }

        @Override
        public boolean inDragRegion(MotionEvent e) {
            return DocumentHolder.this.inDragRegion(e);
        }

        @Override
        public boolean inSelectionHotspot(MotionEvent e) {
            return DocumentHolder.this.inSelectRegion(e);
        }
    }
}
