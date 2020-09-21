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
package com.android.documentsui.selection;

/**
 * Subclass of Selection exposing public support for mutating the underlying selection data.
 * This is useful for clients of {@link SelectionHelper} that wish to manipulate
 * a copy of selection data obtained via {@link SelectionHelper#copySelection(Selection)}.
 */
public final class MutableSelection extends Selection {

    @Override
    public boolean add(String id) {
        return super.add(id);
    }

    @Override
    public boolean remove(String id) {
        return super.remove(id);
    }

    @Override
    public void clear() {
        super.clear();
    }
}
