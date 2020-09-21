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
import android.widget.AbsSpinner;
import android.widget.ArrayAdapter;
import android.widget.SpinnerAdapter;

import java.util.List;

public class AbsSpinnerBindingAdapter {

    @BindingAdapter({"android:entries"})
    public static <T extends CharSequence> void setEntries(AbsSpinner view, T[] entries) {
        if (entries != null) {
            SpinnerAdapter oldAdapter = view.getAdapter();
            boolean changed = true;
            if (oldAdapter != null && oldAdapter.getCount() == entries.length) {
                changed = false;
                for (int i = 0; i < entries.length; i++) {
                    if (!entries[i].equals(oldAdapter.getItem(i))) {
                        changed = true;
                        break;
                    }
                }
            }
            if (changed) {
                ArrayAdapter<CharSequence> adapter =
                        new ArrayAdapter<CharSequence>(view.getContext(),
                                android.R.layout.simple_spinner_item, entries);
                adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                view.setAdapter(adapter);
            }
        } else {
            view.setAdapter(null);
        }
    }

    @BindingAdapter({"android:entries"})
    public static <T> void setEntries(AbsSpinner view, List<T> entries) {
        if (entries != null) {
            SpinnerAdapter oldAdapter = view.getAdapter();
            if (oldAdapter instanceof ObservableListAdapter) {
                ((ObservableListAdapter) oldAdapter).setList(entries);
            } else {
                view.setAdapter(new ObservableListAdapter<T>(view.getContext(), entries,
                        android.R.layout.simple_spinner_item,
                        android.R.layout.simple_spinner_dropdown_item, 0));
            }
        } else {
            view.setAdapter(null);
        }
    }
}
