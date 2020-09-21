/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.server.wm.alertwindowappsdk25;

import android.content.ComponentName;
import android.server.am.component.ComponentsBase;

public class Components extends ComponentsBase {

    public static final ComponentName SDK25_ALERT_WINDOW_TEST_ACTIVITY =
            component(Components.class, "AlertWindowTestActivitySdk25");
}
