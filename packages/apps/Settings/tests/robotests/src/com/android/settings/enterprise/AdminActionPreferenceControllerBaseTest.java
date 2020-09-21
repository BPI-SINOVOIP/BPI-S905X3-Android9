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
 * limitations under the License.
 */

package com.android.settings.enterprise;

import static com.google.common.truth.Truth.assertThat;

import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Date;

@RunWith(SettingsRobolectricTestRunner.class)
public class AdminActionPreferenceControllerBaseTest
    extends AdminActionPreferenceControllerTestBase {

    private Date mDate;

    @Override
    public void setUp() {
        super.setUp();
        mController = new AdminActionPreferenceControllerBaseTestable();
    }

    @Override
    public void setDate(Date date) {
        mDate = date;
    }

    @Test
    public void testIsAvailable() {
        assertThat(mController.isAvailable()).isTrue();
    }

    @Override
    public String getPreferenceKey() {
        return null;
    }

    private class AdminActionPreferenceControllerBaseTestable extends
            AdminActionPreferenceControllerBase {
        AdminActionPreferenceControllerBaseTestable() {
            super(AdminActionPreferenceControllerBaseTest.this.mContext);
        }

        @Override
        protected Date getAdminActionTimestamp() {
            return mDate;
        }

        @Override
        public String getPreferenceKey() {
            return null;
        }
    }
}
