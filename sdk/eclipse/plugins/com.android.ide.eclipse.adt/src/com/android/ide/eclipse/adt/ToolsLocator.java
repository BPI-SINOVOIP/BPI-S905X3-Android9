/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.ide.eclipse.adt;

import com.android.ide.eclipse.ddms.IToolsLocator;

/**
 * Implementation of the com.android.ide.ddms.toolsLocator extension point.
 *
 */
public class ToolsLocator implements IToolsLocator {

    @Override
    public String getAdbLocation() {
        return AdtPlugin.getOsAbsoluteAdb();
    }

    @Override
    public String getHprofConvLocation() {
        return AdtPlugin.getOsAbsoluteHprofConv();
    }

    @Override
    public String getTraceViewLocation() {
        return AdtPlugin.getOsAbsoluteTraceview();
    }
}
