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

package com.android.car.settings.users;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.user.CarUserManagerHelper;
import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserManager;
import android.support.design.widget.TextInputEditText;
import android.widget.Button;

import com.android.car.settings.CarSettingsRobolectricTestRunner;
import com.android.car.settings.R;
import com.android.car.settings.testutils.BaseTestActivity;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;

@RunWith(CarSettingsRobolectricTestRunner.class)
/**
 * Tests for EditUsernameFragment.
 */
public class EditUsernameFragmentTest {
    private BaseTestActivity mTestActivity;
    private CarUserManagerHelper mCarUserManagerHelper;

    @Mock
    private UserManager mUserManager;
    @Mock
    private Context mContext;

    @Before
    public void setUpTestActivity() {
        MockitoAnnotations.initMocks(this);

        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(mUserManager);
        when(mContext.getApplicationContext()).thenReturn(mContext);

        mCarUserManagerHelper = new CarUserManagerHelper(mContext);

        mTestActivity = Robolectric.buildActivity(BaseTestActivity.class)
                .setup()
                .get();
    }

    /**
     * Tests that user name of the profile in question is displayed in the TextInputEditTest field.
     */
    @Test
    public void testUserNameDisplayedInDetails() {
        createEditUsernameFragment(10, "test_user");

        TextInputEditText userNameEditText =
                (TextInputEditText) mTestActivity.findViewById(R.id.user_name_text_edit);
        assertThat(userNameEditText.getText().toString()).isEqualTo("test_user");
    }

    /**
     * Tests that clicking OK saves the new name for the user.
     */
    @Test
    public void testClickingOkSavesNewUserName() {
        createEditUsernameFragment(10, "user_name");
        TextInputEditText userNameEditText =
                (TextInputEditText) mTestActivity.findViewById(R.id.user_name_text_edit);
        Button okButton = (Button) mTestActivity.findViewById(R.id.action_button2);

        userNameEditText.requestFocus();
        userNameEditText.setText("new_user_name");
        okButton.callOnClick();

        verify(mUserManager).setUserName(10, "new_user_name");
    }

    /**
     * Tests that clicking Cancel brings us back to the previous fragment in activity.
     */
    @Test
    public void testClickingCancelInvokesGoingBack() {
        int userId = 10;
        createEditUsernameFragment(userId, "test_user");
        TextInputEditText userNameEditText =
                (TextInputEditText) mTestActivity.findViewById(R.id.user_name_text_edit);
        Button cancelButton = (Button) mTestActivity.findViewById(R.id.action_button1);

        userNameEditText.requestFocus();
        userNameEditText.setText("new_user_name");

        mTestActivity.clearOnBackPressedFlag();
        cancelButton.callOnClick();

        // Back called.
        assertThat(mTestActivity.getOnBackPressedFlag()).isTrue();

        // New user name is not saved.
        verify(mUserManager, never()).setUserName(userId, "new_user_name");
    }

    private void createEditUsernameFragment(int userId, String userName) {
        UserInfo testUser = new UserInfo(userId /* id */, userName, 0 /* flags */);

        EditUsernameFragment fragment = EditUsernameFragment.newInstance(testUser);
        fragment.mCarUserManagerHelper = mCarUserManagerHelper;
        mTestActivity.launchFragment(fragment);
    }
}
