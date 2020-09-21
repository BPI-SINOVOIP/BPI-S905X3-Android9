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
package com.android.server.cts;

import com.android.tradefed.build.IBuildInfo;
import android.os.SystemPropertiesProto;

/**
 * Tests system properties has correct incident filters.
 */
public class SystemPropertiesTest extends ProtoDumpTestCase {
    private static final String TAG = "SystemPropertiesTest";

    static void verifySystemPropertiesProto(SystemPropertiesProto properties, final int filterLevel) {
        // check local tagged field
        if (filterLevel == PRIVACY_LOCAL) {
            assertTrue(properties.getExtraPropertiesCount() > 0);
        } else {
            assertEquals(0, properties.getExtraPropertiesCount());
        }
        // check explicit tagged field, persist.sys.timezone is public prop.
        assertEquals(filterLevel == PRIVACY_AUTO, properties.getPersist().getSysTimezone().isEmpty());
        // check automatic tagged field, ro.build.display.id is public prop.
        assertFalse(properties.getRo().getBuild().getDisplayId().isEmpty());
    }
}
