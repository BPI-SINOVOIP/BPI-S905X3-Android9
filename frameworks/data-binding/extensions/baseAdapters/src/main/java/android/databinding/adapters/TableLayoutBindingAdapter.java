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
package android.databinding.adapters;

import android.databinding.BindingAdapter;
import android.util.SparseBooleanArray;
import android.widget.TableLayout;

import java.util.regex.Pattern;

public class TableLayoutBindingAdapter {

    private static Pattern sColumnPattern = Pattern.compile("\\s*,\\s*");

    private static final int MAX_COLUMNS = 20;

    @BindingAdapter({"android:collapseColumns"})
    public static void setCollapseColumns(TableLayout view, CharSequence columnsStr) {
        SparseBooleanArray columns = parseColumns(columnsStr);
        for (int i = 0; i < MAX_COLUMNS; i++) {
            boolean isCollapsed = columns.get(i, false);
            if (isCollapsed != view.isColumnCollapsed(i)) {
                view.setColumnCollapsed(i, isCollapsed);
            }
        }
    }

    @BindingAdapter({"android:shrinkColumns"})
    public static void setShrinkColumns(TableLayout view, CharSequence columnsStr) {
        if (columnsStr != null && columnsStr.length() > 0 && columnsStr.charAt(0) == '*') {
            view.setShrinkAllColumns(true);
        } else {
            view.setShrinkAllColumns(false);
            SparseBooleanArray columns = parseColumns(columnsStr);
            int columnCount = columns.size();
            for (int i = 0; i < columnCount; i++) {
                int column = columns.keyAt(i);
                boolean shrinkable = columns.valueAt(i);
                if (shrinkable) {
                    view.setColumnShrinkable(column, shrinkable);
                }
            }
        }
    }

    @BindingAdapter({"android:stretchColumns"})
    public static void setStretchColumns(TableLayout view, CharSequence columnsStr) {
        if (columnsStr != null && columnsStr.length() > 0 && columnsStr.charAt(0) == '*') {
            view.setStretchAllColumns(true);
        } else {
            view.setStretchAllColumns(false);
            SparseBooleanArray columns = parseColumns(columnsStr);
            int columnCount = columns.size();
            for (int i = 0; i < columnCount; i++) {
                int column = columns.keyAt(i);
                boolean stretchable = columns.valueAt(i);
                if (stretchable) {
                    view.setColumnStretchable(column, stretchable);
                }
            }
        }
    }

    private static SparseBooleanArray parseColumns(CharSequence sequence) {
        SparseBooleanArray columns = new SparseBooleanArray();
        if (sequence == null) {
            return columns;
        }
        String[] columnDefs = sColumnPattern.split(sequence);

        for (String columnIdentifier : columnDefs) {
            try {
                int columnIndex = Integer.parseInt(columnIdentifier);
                // only valid, i.e. positive, columns indexes are handled
                if (columnIndex >= 0) {
                    // putting true in this sparse array indicates that the
                    // column index was defined in the XML file
                    columns.put(columnIndex, true);
                }
            } catch (NumberFormatException e) {
                // we just ignore columns that don't exist
            }
        }

        return columns;
    }
}
