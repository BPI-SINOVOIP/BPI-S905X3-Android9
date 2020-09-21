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

import com.android.server.wm.AppTransitionProto;
import com.android.server.wm.IdentifierProto;
import com.android.server.wm.RootWindowContainerProto;
import com.android.server.wm.WindowManagerPolicyProto;
import com.android.server.wm.WindowManagerServiceDumpProto;

/**
 * Tests for the WindowManagerService proto dump.
 */
public class WindowManagerIncidentTest extends ProtoDumpTestCase {
    static void verifyWindowManagerServiceDumpProto(WindowManagerServiceDumpProto dump, final int filterLevel) throws Exception {
        verifyWindowManagerPolicyProto(dump.getPolicy(), filterLevel);
        verifyRootWindowContainerProto(dump.getRootWindowContainer(), filterLevel);
        verifyIdentifierProto(dump.getFocusedWindow(), filterLevel);
        verifyIdentifierProto(dump.getInputMethodWindow(), filterLevel);
        verifyAppTransitionProto(dump.getAppTransition(), filterLevel);
    }

    private static void verifyWindowManagerPolicyProto(WindowManagerPolicyProto wmp, final int filterLevel) throws Exception {
        assertTrue(WindowManagerPolicyProto.UserRotationMode.getDescriptor().getValues()
                .contains(wmp.getRotationMode().getValueDescriptor()));
        verifyIdentifierProto(wmp.getFocusedWindow(), filterLevel);
        verifyIdentifierProto(wmp.getTopFullscreenOpaqueWindow(), filterLevel);
        verifyIdentifierProto(wmp.getTopFullscreenOpaqueOrDimmingWindow(), filterLevel);
    }

    private static void verifyIdentifierProto(IdentifierProto ip, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(ip.getTitle().isEmpty());
        }
    }

    private static void verifyRootWindowContainerProto(RootWindowContainerProto rwcp, final int filterLevel) throws Exception {
        for (IdentifierProto ip : rwcp.getWindowsList()) {
            verifyIdentifierProto(ip, filterLevel);
        }
    }

    private static void verifyAppTransitionProto(AppTransitionProto atp, final int filterLevel) throws Exception {
        assertTrue(AppTransitionProto.AppState.getDescriptor().getValues()
                .contains(atp.getAppTransitionState().getValueDescriptor()));
        assertTrue(AppTransitionProto.TransitionType.getDescriptor().getValues()
                .contains(atp.getLastUsedAppTransition().getValueDescriptor()));
    }
}
