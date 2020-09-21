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

package com.android.settings;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.provider.Settings;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowActivity;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowUtils.class)
public class MasterClearTest {

    private static final String TEST_ACCOUNT_TYPE = "android.test.account.type";
    private static final String TEST_CONFIRMATION_PACKAGE = "android.test.conf.pkg";
    private static final String TEST_CONFIRMATION_CLASS = "android.test.conf.pkg.ConfActivity";
    private static final String TEST_ACCOUNT_NAME = "test@example.com";

    @Mock
    private ScrollView mScrollView;
    @Mock
    private LinearLayout mLinearLayout;

    @Mock
    private PackageManager mPackageManager;

    @Mock
    private AccountManager mAccountManager;

    @Mock
    private Activity mMockActivity;

    @Mock
    private Intent mMockIntent;

    private MasterClear mMasterClear;
    private ShadowActivity mShadowActivity;
    private Activity mActivity;
    private View mContentView;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mMasterClear = spy(new MasterClear());
        mActivity = Robolectric.setupActivity(Activity.class);
        mShadowActivity = shadowOf(mActivity);
        mContentView = LayoutInflater.from(mActivity).inflate(R.layout.master_clear, null);

        // Make scrollView only have one child
        when(mScrollView.getChildAt(0)).thenReturn(mLinearLayout);
        when(mScrollView.getChildCount()).thenReturn(1);
    }

    @Test
    public void testShowFinalConfirmation_eraseEsimChecked() {
        final Context context = mock(Context.class);
        when(mMasterClear.getContext()).thenReturn(context);

        mMasterClear.mEsimStorage = mContentView.findViewById(R.id.erase_esim);
        mMasterClear.mExternalStorage = mContentView.findViewById(R.id.erase_external);
        mMasterClear.mEsimStorage.setChecked(true);
        mMasterClear.showFinalConfirmation();

        final ArgumentCaptor<Intent> intent = ArgumentCaptor.forClass(Intent.class);

        verify(context).startActivity(intent.capture());
        assertThat(intent.getValue().getBundleExtra(SettingsActivity.EXTRA_SHOW_FRAGMENT_ARGUMENTS)
            .getBoolean(MasterClear.ERASE_ESIMS_EXTRA, false))
            .isTrue();
    }

    @Test
    public void testShowFinalConfirmation_eraseEsimUnchecked() {
        final Context context = mock(Context.class);
        when(mMasterClear.getContext()).thenReturn(context);

        mMasterClear.mEsimStorage = mContentView.findViewById(R.id.erase_esim);
        mMasterClear.mExternalStorage = mContentView.findViewById(R.id.erase_external);
        mMasterClear.mEsimStorage.setChecked(false);
        mMasterClear.showFinalConfirmation();
        final ArgumentCaptor<Intent> intent = ArgumentCaptor.forClass(Intent.class);

        verify(context).startActivity(intent.capture());
        assertThat(intent.getValue().getBundleExtra(SettingsActivity.EXTRA_SHOW_FRAGMENT_ARGUMENTS)
            .getBoolean(MasterClear.ERASE_ESIMS_EXTRA, false))
            .isFalse();
    }

    @Test
    public void testShowWipeEuicc_euiccDisabled() {
        prepareEuiccState(
                false /* isEuiccEnabled */,
                true /* isEuiccProvisioned */,
                false /* isDeveloper */);
        assertThat(mMasterClear.showWipeEuicc()).isFalse();
    }

    @Test
    public void testShowWipeEuicc_euiccEnabled_unprovisioned() {
        prepareEuiccState(
                true /* isEuiccEnabled */,
                false /* isEuiccProvisioned */,
                false /* isDeveloper */);
        assertThat(mMasterClear.showWipeEuicc()).isFalse();
    }

    @Test
    public void testShowWipeEuicc_euiccEnabled_provisioned() {
        prepareEuiccState(
                true /* isEuiccEnabled */,
                true /* isEuiccProvisioned */,
                false /* isDeveloper */);
        assertThat(mMasterClear.showWipeEuicc()).isTrue();
    }

    @Test
    public void testShowWipeEuicc_developerMode_unprovisioned() {
        prepareEuiccState(
                true /* isEuiccEnabled */,
                false /* isEuiccProvisioned */,
                true /* isDeveloper */);
        assertThat(mMasterClear.showWipeEuicc()).isTrue();
    }

    @Test
    public void testEsimRecheckBoxDefaultChecked() {
        assertThat(((CheckBox) mContentView.findViewById(R.id.erase_esim)).isChecked()).isTrue();
    }

    @Test
    public void testHasReachedBottom_NotScrollDown_returnFalse() {
        initScrollView(100, 0, 200);

        assertThat(mMasterClear.hasReachedBottom(mScrollView)).isFalse();
    }

    @Test
    public void testHasReachedBottom_CanNotScroll_returnTrue() {
        initScrollView(100, 0, 80);

        assertThat(mMasterClear.hasReachedBottom(mScrollView)).isTrue();
    }

    @Test
    public void testHasReachedBottom_ScrollToBottom_returnTrue() {
        initScrollView(100, 100, 200);

        assertThat(mMasterClear.hasReachedBottom(mScrollView)).isTrue();
    }

    @Test
    public void testInitiateMasterClear_inDemoMode_sendsIntent() {
        ShadowUtils.setIsDemoUser(true);

        final ComponentName componentName =
            ComponentName.unflattenFromString("com.android.retaildemo/.DeviceAdminReceiver");
        ShadowUtils.setDeviceOwnerComponent(componentName);

        mMasterClear.mInitiateListener
            .onClick(mContentView.findViewById(R.id.initiate_master_clear));
        final Intent intent = mShadowActivity.getNextStartedActivity();
        assertThat(Intent.ACTION_FACTORY_RESET).isEqualTo(intent.getAction());
        assertThat(componentName).isNotNull();
        assertThat(componentName.getPackageName()).isEqualTo(intent.getPackage());
    }

    @Test
    public void testOnActivityResultInternal_invalideRequest() {
        int invalidRequestCode = -1;
        doReturn(false).when(mMasterClear).isValidRequestCode(eq(invalidRequestCode));

        mMasterClear.onActivityResultInternal(invalidRequestCode, Activity.RESULT_OK, null);

        verify(mMasterClear, times(1)).isValidRequestCode(eq(invalidRequestCode));
        verify(mMasterClear, times(0)).establishInitialState();
        verify(mMasterClear, times(0)).getAccountConfirmationIntent();
        verify(mMasterClear, times(0)).showFinalConfirmation();
    }

    @Test
    public void testOnActivityResultInternal_resultCanceled() {
        doReturn(true).when(mMasterClear).isValidRequestCode(eq(MasterClear.KEYGUARD_REQUEST));
        doNothing().when(mMasterClear).establishInitialState();

        mMasterClear
            .onActivityResultInternal(MasterClear.KEYGUARD_REQUEST, Activity.RESULT_CANCELED, null);

        verify(mMasterClear, times(1)).isValidRequestCode(eq(MasterClear.KEYGUARD_REQUEST));
        verify(mMasterClear, times(1)).establishInitialState();
        verify(mMasterClear, times(0)).getAccountConfirmationIntent();
        verify(mMasterClear, times(0)).showFinalConfirmation();
    }

    @Test
    public void testOnActivityResultInternal_keyguardRequestTriggeringConfirmAccount() {
        doReturn(true).when(mMasterClear).isValidRequestCode(eq(MasterClear.KEYGUARD_REQUEST));
        doReturn(mMockIntent).when(mMasterClear).getAccountConfirmationIntent();
        doNothing().when(mMasterClear).showAccountCredentialConfirmation(eq(mMockIntent));

        mMasterClear
            .onActivityResultInternal(MasterClear.KEYGUARD_REQUEST, Activity.RESULT_OK, null);

        verify(mMasterClear, times(1)).isValidRequestCode(eq(MasterClear.KEYGUARD_REQUEST));
        verify(mMasterClear, times(0)).establishInitialState();
        verify(mMasterClear, times(1)).getAccountConfirmationIntent();
        verify(mMasterClear, times(1)).showAccountCredentialConfirmation(eq(mMockIntent));
    }

    @Test
    public void testOnActivityResultInternal_keyguardRequestTriggeringShowFinal() {
        doReturn(true).when(mMasterClear).isValidRequestCode(eq(MasterClear.KEYGUARD_REQUEST));
        doReturn(null).when(mMasterClear).getAccountConfirmationIntent();
        doNothing().when(mMasterClear).showFinalConfirmation();

        mMasterClear
            .onActivityResultInternal(MasterClear.KEYGUARD_REQUEST, Activity.RESULT_OK, null);

        verify(mMasterClear, times(1)).isValidRequestCode(eq(MasterClear.KEYGUARD_REQUEST));
        verify(mMasterClear, times(0)).establishInitialState();
        verify(mMasterClear, times(1)).getAccountConfirmationIntent();
        verify(mMasterClear, times(1)).showFinalConfirmation();
    }

    @Test
    public void testOnActivityResultInternal_confirmRequestTriggeringShowFinal() {
        doReturn(true).when(mMasterClear)
            .isValidRequestCode(eq(MasterClear.CREDENTIAL_CONFIRM_REQUEST));
        doNothing().when(mMasterClear).showFinalConfirmation();

        mMasterClear.onActivityResultInternal(
            MasterClear.CREDENTIAL_CONFIRM_REQUEST, Activity.RESULT_OK, null);

        verify(mMasterClear, times(1))
            .isValidRequestCode(eq(MasterClear.CREDENTIAL_CONFIRM_REQUEST));
        verify(mMasterClear, times(0)).establishInitialState();
        verify(mMasterClear, times(0)).getAccountConfirmationIntent();
        verify(mMasterClear, times(1)).showFinalConfirmation();
    }

    @Test
    public void testGetAccountConfirmationIntent_unsupported() {
        when(mMasterClear.getActivity()).thenReturn(mActivity);
        /* Using the default resources, account confirmation shouldn't trigger */
        assertThat(mMasterClear.getAccountConfirmationIntent()).isNull();
    }

    @Test
    public void testGetAccountConfirmationIntent_no_relevant_accounts() {
        when(mMasterClear.getActivity()).thenReturn(mMockActivity);
        when(mMockActivity.getString(R.string.account_type)).thenReturn(TEST_ACCOUNT_TYPE);
        when(mMockActivity.getString(R.string.account_confirmation_package))
            .thenReturn(TEST_CONFIRMATION_PACKAGE);
        when(mMockActivity.getString(R.string.account_confirmation_class))
            .thenReturn(TEST_CONFIRMATION_CLASS);

        Account[] accounts = new Account[0];
        when(mMockActivity.getSystemService(Context.ACCOUNT_SERVICE)).thenReturn(mAccountManager);
        when(mAccountManager.getAccountsByType(TEST_ACCOUNT_TYPE)).thenReturn(accounts);
        assertThat(mMasterClear.getAccountConfirmationIntent()).isNull();
    }

    @Test
    public void testGetAccountConfirmationIntent_unresolved() {
        when(mMasterClear.getActivity()).thenReturn(mMockActivity);
        when(mMockActivity.getString(R.string.account_type)).thenReturn(TEST_ACCOUNT_TYPE);
        when(mMockActivity.getString(R.string.account_confirmation_package))
            .thenReturn(TEST_CONFIRMATION_PACKAGE);
        when(mMockActivity.getString(R.string.account_confirmation_class))
            .thenReturn(TEST_CONFIRMATION_CLASS);
        Account[] accounts = new Account[] { new Account(TEST_ACCOUNT_NAME, TEST_ACCOUNT_TYPE) };
        when(mMockActivity.getSystemService(Context.ACCOUNT_SERVICE)).thenReturn(mAccountManager);
        when(mAccountManager.getAccountsByType(TEST_ACCOUNT_TYPE)).thenReturn(accounts);
        // The package manager should not resolve the confirmation intent targeting the non-existent
        // confirmation package.
        when(mMockActivity.getPackageManager()).thenReturn(mPackageManager);
        assertThat(mMasterClear.getAccountConfirmationIntent()).isNull();
    }

    @Test
    public void testTryShowAccountConfirmation_ok() {
        when(mMasterClear.getActivity()).thenReturn(mMockActivity);
        // Only try to show account confirmation if the appropriate resource overlays are available.
        when(mMockActivity.getString(R.string.account_type)).thenReturn(TEST_ACCOUNT_TYPE);
        when(mMockActivity.getString(R.string.account_confirmation_package))
            .thenReturn(TEST_CONFIRMATION_PACKAGE);
        when(mMockActivity.getString(R.string.account_confirmation_class))
            .thenReturn(TEST_CONFIRMATION_CLASS);
        // Add accounts to trigger the search for a resolving intent.
        Account[] accounts = new Account[] { new Account(TEST_ACCOUNT_NAME, TEST_ACCOUNT_TYPE) };
        when(mMockActivity.getSystemService(Context.ACCOUNT_SERVICE)).thenReturn(mAccountManager);
        when(mAccountManager.getAccountsByType(TEST_ACCOUNT_TYPE)).thenReturn(accounts);
        // The package manager should not resolve the confirmation intent targeting the non-existent
        // confirmation package.
        when(mMockActivity.getPackageManager()).thenReturn(mPackageManager);

        ActivityInfo activityInfo = new ActivityInfo();
        activityInfo.packageName = TEST_CONFIRMATION_PACKAGE;
        ResolveInfo resolveInfo = new ResolveInfo();
        resolveInfo.activityInfo = activityInfo;
        when(mPackageManager.resolveActivity(any(), eq(0))).thenReturn(resolveInfo);

        Intent actualIntent = mMasterClear.getAccountConfirmationIntent();
        assertEquals(TEST_CONFIRMATION_PACKAGE, actualIntent.getComponent().getPackageName());
        assertEquals(TEST_CONFIRMATION_CLASS, actualIntent.getComponent().getClassName());
    }

    @Test
    public void testShowAccountCredentialConfirmation() {
        // Finally mock out the startActivityForResultCall
        doNothing().when(mMasterClear)
            .startActivityForResult(eq(mMockIntent), eq(MasterClear.CREDENTIAL_CONFIRM_REQUEST));
        mMasterClear.showAccountCredentialConfirmation(mMockIntent);
        verify(mMasterClear, times(1))
            .startActivityForResult(eq(mMockIntent), eq(MasterClear.CREDENTIAL_CONFIRM_REQUEST));
    }

    @Test
    public void testIsValidRequestCode() {
        assertThat(mMasterClear.isValidRequestCode(MasterClear.KEYGUARD_REQUEST)).isTrue();
        assertThat(mMasterClear.isValidRequestCode(MasterClear.CREDENTIAL_CONFIRM_REQUEST))
            .isTrue();
        assertThat(mMasterClear.isValidRequestCode(0)).isFalse();
    }

    @Test
    public void testOnGlobalLayout_shouldNotRemoveListener() {
        final ViewTreeObserver viewTreeObserver = mock(ViewTreeObserver.class);
        mMasterClear.mScrollView = mScrollView;
        mMasterClear.mInitiateButton = mock(Button.class);
        doReturn(true).when(mMasterClear).hasReachedBottom(any());
        when(mScrollView.getViewTreeObserver()).thenReturn(viewTreeObserver);

        mMasterClear.onGlobalLayout();

        verify(viewTreeObserver, never()).removeOnGlobalLayoutListener(mMasterClear);
    }

    private void prepareEuiccState(
            boolean isEuiccEnabled, boolean isEuiccProvisioned, boolean isDeveloper) {
        doReturn(mActivity).when(mMasterClear).getContext();
        doReturn(isEuiccEnabled).when(mMasterClear).isEuiccEnabled(any());
        ContentResolver cr = mActivity.getContentResolver();
        Settings.Global.putInt(cr, Settings.Global.EUICC_PROVISIONED, isEuiccProvisioned ? 1 : 0);
        Settings.Global.putInt(
                cr, Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, isDeveloper ? 1 : 0);
    }

    private void initScrollView(int height, int scrollY, int childBottom) {
        when(mScrollView.getHeight()).thenReturn(height);
        when(mScrollView.getScrollY()).thenReturn(scrollY);
        when(mLinearLayout.getBottom()).thenReturn(childBottom);
    }
}
