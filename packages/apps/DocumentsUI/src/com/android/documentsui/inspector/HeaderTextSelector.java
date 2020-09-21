/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.documentsui.inspector;

import static com.android.internal.util.Preconditions.checkArgument;

import android.text.Spannable;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

/**
 * Selects all or part of a textview.
 */
 public final class HeaderTextSelector implements ActionMode.Callback {

    private final TextView mText;
    private final Selector mSelector;

    public HeaderTextSelector(TextView text, Selector selector) {
        checkArgument(text != null);
        checkArgument(selector != null);
        mText = text;
        mSelector = selector;
    }

    // An action mode is created when the user selects text. This method is called where
    // we force it to select only the filename in the header. Meaning the name of the file would
    // be selected but the extension would not.
    @Override
    public boolean onCreateActionMode(ActionMode actionMode, Menu menu) {
        CharSequence value = mText.getText();
        if (value instanceof Spannable) {
            mSelector.select((Spannable) value, 0, getLengthOfFilename(value));
        }
        return true;
    }

    @Override
    public boolean onPrepareActionMode(ActionMode actionMode, Menu menu) {
        return true;
    }

    @Override
    public boolean onActionItemClicked(ActionMode actionMode, MenuItem menuItem) {
        return false;
    }

    @Override
    public void onDestroyActionMode(ActionMode actionMode) {}

    /**
     * Gets the length of the inspector header for selection.
     *
     * Example:
     * filename.txt returns the end index needed to select "filename".
     *
     * @return the index of the last character in the filename
     */
    private static int getLengthOfFilename(CharSequence text) {
        String title = text.toString();
        if (title != null) {
            int index = title.indexOf('.');
            if (index > 0) {
                return index;
            }
        }

        return text.length();
    }

    public interface Selector {
        void select(Spannable text, int start, int stop);
    }
}
