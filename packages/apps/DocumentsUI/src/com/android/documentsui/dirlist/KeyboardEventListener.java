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
package com.android.documentsui.dirlist;

import android.view.KeyEvent;

import com.android.documentsui.selection.ItemDetailsLookup;
import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

/**
 * KeyboardListener is implemented by {@link KeyInputHandler}. The handler
 * must be called from RecyclerView.Holders in response to keyboard events.
 */
public abstract class KeyboardEventListener {

    /**
     * Handles key events on the view holder.
     *
     * @param details The target ItemHolder.
     * @param keyCode Key code for the event.
     * @param event KeyEvent for the event.
     *
     * @return Whether the event was handled.
     */
    public abstract boolean onKey(ItemDetails details, int keyCode, KeyEvent event);
}
