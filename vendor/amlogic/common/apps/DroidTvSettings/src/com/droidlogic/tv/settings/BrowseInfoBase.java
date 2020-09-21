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
 * limitations under the License.
 */

package com.droidlogic.tv.settings;

import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.HeaderItem;
import android.support.v17.leanback.widget.ListRow;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import android.util.SparseArray;

import java.util.ArrayList;

public abstract class BrowseInfoBase implements BrowseInfoFactory {
    protected final SparseArray<ArrayObjectAdapter> mRows = new SparseArray<ArrayObjectAdapter>();
    protected final ArrayList<HeaderItem> mHeaderItems = new ArrayList<HeaderItem>();
    private final ClassPresenterSelector mPresenterSelector = new ClassPresenterSelector();

    public BrowseInfoBase() {
        mPresenterSelector.addClassPresenter(MenuItem.class, new MenuItemPresenter());
    }

    @Override
    public ArrayObjectAdapter getRows() {
        ArrayObjectAdapter rows = new ArrayObjectAdapter();

        for (int i = 0, size = mHeaderItems.size(); i < size; i++) {
            HeaderItem headerItem = mHeaderItems.get(i);
            ArrayObjectAdapter adapter = mRows.get((int) headerItem.getId());
            adapter.setPresenterSelector(mPresenterSelector);
            rows.add(new ListRow(headerItem, adapter));
        }
        return rows;
    }

}
