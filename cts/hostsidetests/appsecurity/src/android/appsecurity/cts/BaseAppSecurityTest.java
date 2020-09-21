/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package android.appsecurity.cts;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.Assert;
import org.junit.Before;

import java.util.ArrayList;

/**
 * Base class.
 */
abstract class BaseAppSecurityTest extends BaseHostJUnit4Test {

    /** Whether multi-user is supported. */
    protected boolean mSupportsMultiUser;
    protected boolean mIsSplitSystemUser;
    protected int mPrimaryUserId;
    /** Users we shouldn't delete in the tests */
    private ArrayList<Integer> mFixedUsers;

    @Before
    public void setUp() throws Exception {
        Assert.assertNotNull(getBuild()); // ensure build has been set before test is run.

        mSupportsMultiUser = getDevice().getMaxNumberOfUsersSupported() > 1;
        mIsSplitSystemUser = checkIfSplitSystemUser();
        mPrimaryUserId = getDevice().getPrimaryUserId();
        mFixedUsers = new ArrayList<>();
        mFixedUsers.add(mPrimaryUserId);
        if (mPrimaryUserId != Utils.USER_SYSTEM) {
            mFixedUsers.add(Utils.USER_SYSTEM);
        }
        getDevice().switchUser(mPrimaryUserId);
    }

    private boolean checkIfSplitSystemUser() throws DeviceNotAvailableException {
        final String commandOuput = getDevice().executeShellCommand(
                "getprop ro.fw.system_user_split");
        return "y".equals(commandOuput) || "yes".equals(commandOuput)
                || "1".equals(commandOuput) || "true".equals(commandOuput)
                || "on".equals(commandOuput);
    }

    protected void installTestAppForUser(String apk, int userId) throws Exception {
        if (userId < 0) {
            userId = mPrimaryUserId;
        }
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(getBuild());
        Assert.assertNull(getDevice().installPackageForUser(
                buildHelper.getTestFile(apk), true, false, userId, "-t"));
    }

    protected boolean isAppVisibleForUser(String packageName, int userId,
            boolean matchUninstalled) throws DeviceNotAvailableException {
        String command = "cmd package list packages --user " + userId;
        if (matchUninstalled) command += " -u";
        String output = getDevice().executeShellCommand(command);
        return output.contains(packageName);
    }
}
