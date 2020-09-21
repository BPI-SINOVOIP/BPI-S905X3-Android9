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

package com.android.documentsui.testing;

import android.content.Context;
import android.graphics.drawable.Drawable;

import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.dirlist.IconHelper;

import org.mockito.Mockito;

public class TestIconHelper extends IconHelper {

    public Drawable nextDocumentIcon;

    private TestIconHelper() {
        super(null, 0);
    }

    @Override
    public Drawable getDocumentIcon(Context context, DocumentInfo doc) {
        return nextDocumentIcon;
    }

    public static TestIconHelper create() {
        return Mockito.mock(TestIconHelper.class, Mockito.CALLS_REAL_METHODS);
    }
}
