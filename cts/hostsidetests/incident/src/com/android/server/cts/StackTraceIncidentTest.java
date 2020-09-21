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

import android.os.BackTraceProto;

/**
 * Tests for trace proto dumping.
 */
public class StackTraceIncidentTest extends ProtoDumpTestCase {
    static void verifyBackTraceProto(BackTraceProto dump, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            for (BackTraceProto.Stack s : dump.getTracesList()) {
                // The entire message is tagged as EXPLICIT, so even the pid should be stripped out.
                assertTrue(s.getPid() == 0);
                assertTrue(s.getDump().isEmpty());
                // The entire message is tagged as EXPLICIT, so even the dump duration should be stripped out.
                assertTrue(s.getDumpDurationNs() == 0);
            }
        }
    }
}
