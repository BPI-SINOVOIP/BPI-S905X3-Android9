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

package android.signature.cts.api;

import android.signature.cts.DexMemberChecker;

public class WildcardKillswitchTest extends BaseKillswitchTest {
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        String exemptions = getGlobalExemptions();
        mErrorMessageAppendix = " when global setting hidden_api_blacklist_exemptions is "
            + "\"" + exemptions + "\"";

        // precondition check: make sure global setting has been configured properly.
        // This should be done via an adb command, configured in AndroidTest.xml.
        assertEquals("*", exemptions);
    }
}
