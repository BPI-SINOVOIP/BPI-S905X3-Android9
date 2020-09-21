/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.ide.eclipse.adt.internal.editors.layout.properties;

import com.android.annotations.NonNull;
import com.android.annotations.Nullable;
import com.android.ide.eclipse.adt.internal.editors.common.CommonXmlEditor;
import com.android.ide.eclipse.adt.internal.editors.descriptors.AttributeDescriptor;

class PropertyValueCompleter extends ValueCompleter {
    private final XmlProperty mProperty;

    PropertyValueCompleter(XmlProperty property) {
        mProperty = property;
    }

    @Override
    @Nullable
    protected CommonXmlEditor getEditor() {
        return mProperty.getXmlEditor();
    }

    @Override
    @NonNull
    protected AttributeDescriptor getDescriptor() {
        return mProperty.getDescriptor();
    }
}
