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
package com.android.compatibility.common.util;

import org.junit.Before;

/**
 *  Device-side base class for tests leveraging the Business Logic service for rules that are
 *  conditionally added based on the device characteristics.
 */
public class BusinessLogicConditionalTestCase extends BusinessLogicTestCase {

    @Override
    @Before
    public void handleBusinessLogic() {
        super.loadBusinessLogic();
        ensureAuthenticated();
        super.executeBusinessLogic();
    }

    protected void ensureAuthenticated() {
        if (!mCanReadBusinessLogic) {
            // super class handles the condition that the service is unavailable.
            return;
        }

        if (!mBusinessLogic.mConditionalTestsEnabled) {
            skipTest("Execution of device specific tests is not enabled. "
                    + "Enable with '--conditional-business-logic-tests-enabled'");
        }

        if (mBusinessLogic.isAuthorized()) {
            // Run test as normal.
            return;
        }
        String message = mBusinessLogic.getAuthenticationStatusMessage();

        // Fail test since request was not authorized.
        failTest(String.format("Unable to execute because %s.", message));
    }
}
