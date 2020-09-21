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

package com.android.car.setupwizardlib;

import static com.google.common.truth.Truth.assertThat;

import android.app.Activity;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import com.android.car.setupwizardlib.robolectric.BaseRobolectricTest;
import com.android.car.setupwizardlib.robolectric.CarSetupWizardLibRobolectricTestRunner;
import com.android.car.setupwizardlib.robolectric.TestHelper;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.robolectric.Robolectric;

/**
 * Unit tests for the {@link BaseActivity}.
 */
@RunWith(CarSetupWizardLibRobolectricTestRunner.class)
public class BaseActivityTest extends BaseRobolectricTest {
    BaseActivity mBaseActivity;
    CarSetupWizardLayout mCarSetupWizardLayout;

    @Before
    public void setupBaseActivityAndLayout() {
        mBaseActivity = (BaseActivity) Robolectric.buildActivity(BaseActivity.class).create().get();
        mCarSetupWizardLayout = mBaseActivity.getCarSetupWizardLayout();
    }

    /**
     * Test that the BaseActivity's content view is set to be a CarSetupWizardLayout
     */
    @Test
    public void testContentViewIsCarSetupWizardLayout() {
        View contentView = mBaseActivity.findViewById(R.id.car_setup_wizard_layout);
        assertThat(contentView).isNotNull();
        assertThat(contentView instanceof CarSetupWizardLayout).isTrue();
    }

    private BaseActivity createSpyBaseActivity() {
        BaseActivity spyBaseActivity = Mockito.spy(
                (BaseActivity) Robolectric.buildActivity(BaseActivity.class).get());
        spyBaseActivity.onCreate(null);

        return spyBaseActivity;
    }

    /**
     * Test that the BaseActivity sets the back button listener to call
     * {@link BaseActivity#handleBackButton()} when created.
     */
    @Test
    public void testBackButtonListenerIsDefault() {
        BaseActivity spyBaseActivity = createSpyBaseActivity();

        ImageView backButton = (ImageView) spyBaseActivity.findViewById(
                R.id.back_button);
        backButton.performClick();

        Mockito.verify(spyBaseActivity).handleBackButton();
    }

    /**
     * Test that the BaseActivity sets the secondary toolbar button listener to the default when
     * created.
     */
    @Test
    public void testSecondaryToolbarButtonListenerIsDefault() {
        BaseActivity spyBaseActivity = createSpyBaseActivity();

        Button secondaryToolBarButton = (Button) spyBaseActivity.findViewById(
                R.id.secondary_toolbar_button);
        secondaryToolBarButton.performClick();

        Mockito.verify(spyBaseActivity).nextAction(Activity.RESULT_OK);
    }


    /**
     * Test that the BaseActivity sets the primary toolbar button listener to the default when
     * created.
     */
    @Test
    public void testPrimaryToolbarButtonListenerIsDefault() {
        BaseActivity spyBaseActivity = createSpyBaseActivity();

        Button primaryToolBarButton = (Button) spyBaseActivity.findViewById(
                R.id.primary_toolbar_button);
        primaryToolBarButton.performClick();

        Mockito.verify(spyBaseActivity).nextAction(Activity.RESULT_OK);
    }

    private BaseActivity getStartedBaseActivity() {
        return (BaseActivity) Robolectric.buildActivity(BaseActivity.class).create().start().get();
    }

    private BaseActivity getSavedInstanceStateBaseActivity() {
        return (BaseActivity) Robolectric.buildActivity(
                BaseActivity.class).create().saveInstanceState(new Bundle()).get();
    }

    /**
     * Test that fragment commits are allowed after {@link BaseActivity#onStart()} is called.
     */
    @Test
    public void testFragmentCommitsAllowedAfterOnStart() {
        assertThat(getStartedBaseActivity().getAllowFragmentCommits()).isTrue();
    }

    /**
     * Test that fragment commits are not allowed after {@link BaseActivity#onSaveInstanceState }
     * is called.
     */
    @Test
    public void testFragmentCommitsNotAllowedAfterOnSavedInstanceState() {
        assertThat(getSavedInstanceStateBaseActivity().getAllowFragmentCommits()).isFalse();
    }


    /**
     * Test that {@link BaseActivity#setContentFragment} sets the content fragment and calls the
     * expected methods when fragment commits are allowed.
     */
    @Test
    public void testSetContentFragmentWhenFragmentCommitsAllowed() {
        BaseActivity spyBaseActivity = Mockito.spy(getStartedBaseActivity());

        Fragment fragment = new Fragment();
        spyBaseActivity.setContentFragment(fragment);

        assertThat(spyBaseActivity.getContentFragment()).isEqualTo(fragment);
        assertThat(spyBaseActivity.getSupportFragmentManager().getBackStackEntryCount()).isEqualTo(
                0);
        // Verify that onContentFragmentSet is called with the test fragment
        Mockito.verify(spyBaseActivity).onContentFragmentSet(fragment);
    }

    /**
     * Test that {@link BaseActivity#setContentFragment} does nothing when fragment commits are not
     * allowed.
     */
    @Test
    public void testSetContentFragmentWhenFragmentCommitsNotAllowed() {
        BaseActivity spyBaseActivity = Mockito.spy(getSavedInstanceStateBaseActivity());

        Fragment fragment = new Fragment();
        spyBaseActivity.setContentFragment(fragment);

        assertThat(spyBaseActivity.getContentFragment()).isEqualTo(null);
        assertThat(spyBaseActivity.getSupportFragmentManager().getBackStackEntryCount()).isEqualTo(
                0);
        // Verify that onContentFragmentSet is not called
        Mockito.verify(spyBaseActivity, Mockito.times(0)).onContentFragmentSet(fragment);
    }

    /**
     * Test that {@link BaseActivity#setContentFragmentWithBackstack)} sets the content fragment,
     * adds it to the fragment backstack, and calls the expected methods when fragment commits
     * are allowed.
     */
    @Test
    public void testSetContentFragmentWithBackstackWhenFragmentCommitsAllowed() {
        BaseActivity spyBaseActivity = Mockito.spy(getStartedBaseActivity());

        Fragment fragment = new Fragment();
        spyBaseActivity.setContentFragmentWithBackstack(fragment);

        assertThat(spyBaseActivity.getContentFragment()).isEqualTo(fragment);
        assertThat(spyBaseActivity.getSupportFragmentManager().getBackStackEntryCount()).isEqualTo(
                1);
        // Verify that onContentFragmentSet is called with the test fragment
        Mockito.verify(spyBaseActivity).onContentFragmentSet(fragment);
    }

    /**
     * Test that {@link BaseActivity#setContentFragmentWithBackstack)} does nothing when fragment
     * commits are not allowed.
     */
    @Test
    public void testSetContentFragmentWithBackstackWhenFragmentCommitsNotAllowed() {
        BaseActivity spyBaseActivity = Mockito.spy(getSavedInstanceStateBaseActivity());

        Fragment fragment = new Fragment();
        spyBaseActivity.setContentFragment(fragment);

        assertThat(spyBaseActivity.getContentFragment()).isEqualTo(null);
        assertThat(spyBaseActivity.getSupportFragmentManager().getBackStackEntryCount()).isEqualTo(
                0);
        // Verify that onContentFragmentSet is not called
        Mockito.verify(spyBaseActivity, Mockito.times(0)).onContentFragmentSet(fragment);
    }

    /**
     * Test that {@link BaseActivity#popBackStackImmediate()} returns false when no fragment is
     * added to the backstack.
     */
    @Test
    public void testPopBackStackImmediateWithEmptyStack() {
        assertThat(mBaseActivity.popBackStackImmediate()).isEqualTo(false);
    }

    /**
     * Test that {@link BaseActivity#popBackStackImmediate()} returns true when a fragment is
     * added to the backstack and that the fragment is popped off of the backstack.
     */
    @Test
    public void testPopBackStackImmediateWithFragmentInStack() {
        Fragment fragment = new Fragment();
        mBaseActivity.setContentFragmentWithBackstack(fragment);
        assertThat(mBaseActivity.popBackStackImmediate()).isEqualTo(true);

        assertThat(mBaseActivity.getContentFragment()).isNull();
    }

    /**
     * Test that {@link BaseActivity#getContentFragment()} returns the content fragment.
     */
    @Test
    public void testGetContentFragment() {
        Fragment fragment = new Fragment();
        mBaseActivity.setContentFragment(fragment);

        assertThat(mBaseActivity.getContentFragment()).isEqualTo(
                mBaseActivity.getSupportFragmentManager().findFragmentByTag(
                        mBaseActivity.CONTENT_FRAGMENT_TAG));
    }

    /**
     * Test that {@link BaseActivity#setContentLayout} adds the specified layout to the
     * BaseActivity.
     */
    @Test
    public void testSetContentLayout() {
        mBaseActivity.setContentLayout(R.layout.base_activity_test_layout);
        View contentLayout = mBaseActivity.findViewById(R.id.content_layout);
        assertThat(contentLayout).isNotNull();
    }

    /**
     * Test that {@link BaseActivity#finishAction()} results in a call to
     * {@link BaseActivity#finish}.
     */
    @Test
    public void testFinishAction() {
        BaseActivity spyBaseActivity = Mockito.spy(mBaseActivity);
        spyBaseActivity.finishAction();

        Mockito.verify(spyBaseActivity).finish();
    }

    /**
     * Test that {@link BaseActivity#finishAction(int)} )} results in a call to
     * {@link BaseActivity#nextAction} and {@link BaseActivity#finish}.
     */
    @Test
    public void testFinishActionWithResultCode() {
        BaseActivity spyBaseActivity = Mockito.spy(mBaseActivity);
        spyBaseActivity.finishAction(BaseActivity.RESULT_OK);

        Mockito.verify(spyBaseActivity).nextAction(BaseActivity.RESULT_OK, null);
        Mockito.verify(spyBaseActivity).finish();
    }

    /**
     * Test that {@link BaseActivity#setBackButtonVisible} sets the back button visible/not visible.
     */
    @Test
    public void testSetBackButtonVisibleTrue() {
        mBaseActivity.setBackButtonVisible(true);
        TestHelper.assertViewVisible(mCarSetupWizardLayout.getBackButton());
    }

    /**
     * Test that {@link BaseActivity#setBackButtonVisible} sets the back button visible/not visible.
     */
    @Test
    public void testSetBackButtonVisibleFalse() {
        mBaseActivity.setBackButtonVisible(false);
        TestHelper.assertViewNotVisible(mCarSetupWizardLayout.getBackButton());
    }

    /**
     * Test that {@link BaseActivity#setToolbarTitleVisible} sets the toolbar title visible/not
     * visible.
     */
    @Test
    public void testSetToolbarTitleVisibleTrue() {
        mBaseActivity.setToolbarTitleVisible(true);
        TestHelper.assertViewVisible(mCarSetupWizardLayout.getToolbarTitle());
    }

    /**
     * Test that {@link BaseActivity#setToolbarTitleVisible} sets the toolbar button visible/not
     * visible.
     */
    @Test
    public void testSetToolbarTitleVisibleFalse() {
        mBaseActivity.setToolbarTitleVisible(false);
        TestHelper.assertViewNotVisible(mCarSetupWizardLayout.getToolbarTitle());
    }

    /**
     * Test that {@link BaseActivity#setToolbarTitleText(String)} sets the toolbar title text.
     */
    @Test
    public void testSetToolbarTitleText() {
        mBaseActivity.setToolbarTitleText("title text");
        TestHelper.assertTextEqual(mCarSetupWizardLayout.getToolbarTitle(), "title text");
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonVisible} sets the primary toolbar button
     * visible/not visible.
     */
    @Test
    public void testSetPrimaryToolbarButtonVisibleTrue() {
        mBaseActivity.setPrimaryToolbarButtonVisible(true);
        TestHelper.assertViewVisible(mCarSetupWizardLayout.getPrimaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonVisible} sets the primary toolbar button
     * visible/not visible.
     */
    @Test
    public void testSetPrimaryToolbarButtonVisibleFalse() {
        mBaseActivity.setPrimaryToolbarButtonVisible(false);
        TestHelper.assertViewNotVisible(mCarSetupWizardLayout.getPrimaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonEnabled} sets the primary toolbar button
     * visible/not visible.
     */
    @Test
    public void testSetPrimaryToolbarButtonEnabledTrue() {
        mBaseActivity.setPrimaryToolbarButtonEnabled(true);
        TestHelper.assertViewEnabled(mCarSetupWizardLayout.getPrimaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonEnabled} sets the primary toolbar button
     * visible/not visible.
     */
    @Test
    public void testSetPrimaryToolbarButtonEnabledFalse() {
        mBaseActivity.setPrimaryToolbarButtonEnabled(false);
        TestHelper.assertViewNotEnabled(mCarSetupWizardLayout.getPrimaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonText(String)} sets the primary
     * toolbar title text.
     */
    @Test
    public void testSetPrimaryToolbarButtonText() {
        mBaseActivity.setPrimaryToolbarButtonText("button text");
        TestHelper.assertTextEqual(mCarSetupWizardLayout.getPrimaryToolbarButton(), "button text");
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonFlat(boolean)} sets the primary toolbar
     * button flat/not flat.
     */
    @Test
    public void testSetPrimaryToolbarButtonFlatTrue() {
        mBaseActivity.setPrimaryToolbarButtonFlat(true);
        assertThat(mCarSetupWizardLayout.getPrimaryToolbarButtonFlat()).isTrue();
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonFlat(boolean)} sets the primary toolbar
     * button flat/not flat.
     */
    @Test
    public void testSetPrimaryToolbarButtonFlatFalse() {
        mBaseActivity.setPrimaryToolbarButtonFlat(false);
        assertThat(mCarSetupWizardLayout.getPrimaryToolbarButtonFlat()).isFalse();
    }

    /**
     * Test that {@link BaseActivity#setPrimaryToolbarButtonOnClickListener} sets the primary
     * toolbar button's click listener.
     */
    @Test
    public void testSetPrimaryToolbarButtonOnClickListener() {
        View.OnClickListener spyListener = TestHelper.createSpyListener();

        mBaseActivity.setPrimaryToolbarButtonOnClickListener(spyListener);
        mBaseActivity.getCarSetupWizardLayout().getPrimaryToolbarButton().performClick();
        Mockito.verify(spyListener).onClick(Mockito.any());
    }

    /**
     * Test that {@link BaseActivity#setSecondaryToolbarButtonVisible} sets the secondary toolbar
     * button visible/not visible.
     */
    @Test
    public void testSetSecondaryToolbarButtonVisibleTrue() {
        mBaseActivity.setSecondaryToolbarButtonVisible(true);
        TestHelper.assertViewVisible(mCarSetupWizardLayout.getSecondaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setSecondaryToolbarButtonVisible} sets the secondary toolbar
     * button visible/not visible.
     */
    @Test
    public void testSetSecondaryToolbarButtonVisibleFalse() {
        mBaseActivity.setSecondaryToolbarButtonVisible(false);
        TestHelper.assertViewNotVisible(mCarSetupWizardLayout.getSecondaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setSecondaryToolbarButtonEnabled} sets the secondary toolbar
     * button visible/not visible.
     */
    @Test
    public void testSetSecondaryToolbarButtonEnabledTrue() {
        mBaseActivity.setSecondaryToolbarButtonEnabled(true);
        TestHelper.assertViewEnabled(mCarSetupWizardLayout.getSecondaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setSecondaryToolbarButtonEnabled} sets the secondary toolbar
     * button visible/not visible.
     */
    @Test
    public void testSetSecondaryToolbarButtonEnabledFalse() {
        mBaseActivity.setSecondaryToolbarButtonEnabled(false);
        TestHelper.assertViewNotEnabled(mCarSetupWizardLayout.getSecondaryToolbarButton());
    }

    /**
     * Test that {@link BaseActivity#setSecondaryToolbarButtonText(String)} sets the secondary
     * toolbar title text.
     */
    @Test
    public void testSetSecondaryToolbarButtonText() {
        mBaseActivity.setSecondaryToolbarButtonText("button text");
        TestHelper.assertTextEqual(mCarSetupWizardLayout.getSecondaryToolbarButton(),
                "button text");
    }

    /**
     * Test that {@link BaseActivity#setSecondaryToolbarButtonOnClickListener} sets the secondary
     * toolbar button's click listener.
     */
    @Test
    public void testSetSecondaryToolbarButtonOnClickListener() {
        View.OnClickListener spyListener = TestHelper.createSpyListener();

        mBaseActivity.setSecondaryToolbarButtonOnClickListener(spyListener);
        mBaseActivity.getCarSetupWizardLayout().getSecondaryToolbarButton().performClick();
        Mockito.verify(spyListener).onClick(Mockito.any());
    }

    /**
     * Test that {@link BaseActivity#setProgressBarVisible} sets the progressbar visible/not
     * visible.
     */
    @Test
    public void testSetProgressBarVisibleTrue() {
        mBaseActivity.setProgressBarVisible(true);
        TestHelper.assertViewVisible(mCarSetupWizardLayout.getProgressBar());
    }

    /**
     * Test that {@link BaseActivity#setProgressBarVisible} sets the progressbar visible/not
     * visible.
     */
    @Test
    public void testSetProgressBarVisibleFalse() {
        mBaseActivity.setProgressBarVisible(false);
        TestHelper.assertViewNotVisible(mCarSetupWizardLayout.getProgressBar());
    }
}
