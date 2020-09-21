/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.renderscriptlegacy.cts;

import android.renderscript.RenderScript.RSMessageHandler;
import android.renderscript.RSRuntimeException;

/**
 * Test for appropriate handling of versioned bitcode.
 */
public class VersionTest extends RSBaseCompute {
    public void testVersion11() {
        ScriptC_set_target_api_11 test11 = new ScriptC_set_target_api_11(mRS);
        test11.invoke_check(11);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion12() {
        ScriptC_set_target_api_12 test12 = new ScriptC_set_target_api_12(mRS);
        test12.invoke_check(12);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion13() {
        ScriptC_set_target_api_13 test13 = new ScriptC_set_target_api_13(mRS);
        test13.invoke_check(13);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion14() {
        ScriptC_set_target_api_14 test14 = new ScriptC_set_target_api_14(mRS);
        test14.invoke_check(14);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion15() {
        ScriptC_set_target_api_15 test15 = new ScriptC_set_target_api_15(mRS);
        test15.invoke_check(15);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion16() {
        ScriptC_set_target_api_16 test16 = new ScriptC_set_target_api_16(mRS);
        test16.invoke_check(16);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion17() {
        ScriptC_set_target_api_17 test17 = new ScriptC_set_target_api_17(mRS);
        test17.invoke_check(17);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion18() {
        ScriptC_set_target_api_18 test18 = new ScriptC_set_target_api_18(mRS);
        test18.invoke_check(18);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion19() {
        ScriptC_set_target_api_19 test19 = new ScriptC_set_target_api_19(mRS);
        test19.invoke_check(19);
        waitForMessage();
        checkForErrors();
    }

    public void testVersion_too_high() {
        try {
            ScriptC_set_target_api_too_high test_too_high =
                    new ScriptC_set_target_api_too_high(mRS);
            fail("should throw RSRuntimeException");
        } catch (RSRuntimeException e) {
        }
        checkForErrors();
    }
}
