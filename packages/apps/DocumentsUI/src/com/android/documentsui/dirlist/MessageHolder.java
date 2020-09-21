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
 * limitations under the License.
 */

package com.android.documentsui.dirlist;

import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

import android.content.Context;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.widget.Space;

/**
 * Base class for all non-Document Holder classes.
 */
abstract class MessageHolder extends DocumentHolder {
    public MessageHolder(Context context, Space space) {
        super(context, space);
    }

    public MessageHolder(Context context, ViewGroup parent, int layout) {
        super(context, parent, layout);
    }

    @Override
    public ItemDetails getItemDetails() {
        return null;
    }
}
