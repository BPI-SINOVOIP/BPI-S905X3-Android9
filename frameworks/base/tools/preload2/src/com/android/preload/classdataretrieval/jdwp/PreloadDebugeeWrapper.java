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

package com.android.preload.classdataretrieval.jdwp;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPManualDebuggeeWrapper;
import org.apache.harmony.jpda.tests.share.JPDATestOptions;

import java.io.IOException;

public class PreloadDebugeeWrapper extends JDWPManualDebuggeeWrapper {

    public PreloadDebugeeWrapper(JPDATestOptions options, LogWriter writer) {
        super(options, writer);
    }

    @Override
    protected Process launchProcess(String cmdLine) throws IOException {
        return null;
    }

    @Override
    protected void WaitForProcessExit(Process process) {
    }

}
