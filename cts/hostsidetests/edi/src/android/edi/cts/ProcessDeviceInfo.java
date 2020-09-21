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
package android.edi.cts;

import com.android.compatibility.common.util.DeviceInfo;
import com.android.compatibility.common.util.HostInfoStore;
import com.android.compatibility.common.util.RootProcessScanner;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Set;

public class ProcessDeviceInfo extends DeviceInfo {

    @Override
    protected void collectDeviceInfo(HostInfoStore store) throws Exception {
        RootProcessScanner scanner = new RootProcessScanner(getDevice());
        Set<String> approved = scanner.getRootProcesses(true);
        store.addListResult("approved_root_processes", new ArrayList<String>(approved));
        Set<String> unapproved = scanner.getRootProcesses(false);
        store.addListResult("unapproved_root_processes", new ArrayList<String>(unapproved));
    }
}
