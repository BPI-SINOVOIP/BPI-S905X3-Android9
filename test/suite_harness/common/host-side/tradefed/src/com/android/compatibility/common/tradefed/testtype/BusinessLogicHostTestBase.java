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
package com.android.compatibility.common.tradefed.testtype;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TestName;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.util.BusinessLogic;
import com.android.compatibility.common.util.BusinessLogicExecutor;
import com.android.compatibility.common.util.BusinessLogicFactory;
import com.android.compatibility.common.util.BusinessLogicHostExecutor;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.testtype.suite.TestSuiteInfo;

import java.io.File;

/**
 * Host-side base class for tests leveraging the Business Logic service.
 */
public class BusinessLogicHostTestBase extends BaseHostJUnit4Test {

    /* String marking the beginning of the parameter in a test name */
    private static final String PARAM_START = "[";

    /* Test name rule that tracks the current test method under execution */
    @Rule public TestName mTestCase = new TestName();

    protected BusinessLogic mBusinessLogic;
    protected boolean mCanReadBusinessLogic = true;

    @Before
    public void handleBusinessLogic() {
        loadBusinessLogic();
        executeBusinessLogic();
    }

    protected void executeBusinessLogic() {
        String methodName = mTestCase.getMethodName();
        assertTrue(String.format("Test \"%s\" is unable to execute as it depends on the missing "
                + "remote configuration.", methodName), mCanReadBusinessLogic);
        if (methodName.contains(PARAM_START)) {
            // Strip parameter suffix (e.g. "[0]") from method name
            methodName = methodName.substring(0, methodName.lastIndexOf(PARAM_START));
        }
        String testName = String.format("%s#%s", this.getClass().getName(), methodName);
        if (mBusinessLogic.hasLogicFor(testName)) {
            CLog.i("Applying business logic for test case: ", testName);
            BusinessLogicExecutor executor = new BusinessLogicHostExecutor(getDevice(),
                    getBuild(), this);
            mBusinessLogic.applyLogicFor(testName, executor);
        }
    }

    protected void loadBusinessLogic() {
        CompatibilityBuildHelper helper = new CompatibilityBuildHelper(getBuild());
        File businessLogicFile = helper.getBusinessLogicHostFile();
        if (businessLogicFile != null && businessLogicFile.canRead()) {
            mBusinessLogic = BusinessLogicFactory.createFromFile(businessLogicFile);
        } else {
            mCanReadBusinessLogic = false; // failed to retrieve business logic
        }
    }

    public static void skipTest(String message) {
        assumeTrue(message, false);
    }

    public static void failTest(String message) {
        fail(message);
    }
}

