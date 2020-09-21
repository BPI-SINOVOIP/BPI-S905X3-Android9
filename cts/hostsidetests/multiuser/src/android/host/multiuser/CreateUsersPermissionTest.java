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
package android.host.multiuser;

import static com.android.tradefed.log.LogUtil.CLog;

import com.android.ddmlib.Log;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(DeviceJUnit4ClassRunner.class)
public class CreateUsersPermissionTest extends BaseMultiUserTest {

    @Test
    public void testCanCreateGuestUser() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }
        getDevice().createUser(
                "TestUser_" + System.currentTimeMillis() /* name */,
                true /* guest */,
                false /* ephemeral */);
    }

    @Test
    public void testCanCreateEphemeralUser() throws Exception {
        if (!mSupportsMultiUser || !mIsSplitSystemUser) {
            return;
        }
        getDevice().createUser(
                "TestUser_" + System.currentTimeMillis() /* name */,
                false /* guest */,
                true /* ephemeral */);
    }

    @Test
    public void testCanCreateRestrictedUser() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }
        createRestrictedProfile(mPrimaryUserId);
    }

    @Test
    public void testCantSetUserRestriction() throws Exception {
        if (getDevice().isAdbRoot()) {
            CLog.logAndDisplay(Log.LogLevel.WARN,
                    "Cannot test testCantSetUserRestriction on rooted devices");
            return;
        }
        final String setRestriction = "pm set-user-restriction no_fun ";
        final String output = getDevice().executeShellCommand(setRestriction + "1");
        final boolean isErrorOutput = output.contains("SecurityException")
            && output.contains("You need MANAGE_USERS permission");
        Assert.assertTrue("Trying to set user restriction should fail with SecurityException. "
                + "command output: " + output, isErrorOutput);
    }
}
