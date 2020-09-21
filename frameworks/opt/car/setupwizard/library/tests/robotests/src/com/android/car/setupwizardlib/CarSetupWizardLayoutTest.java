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

package com.android.car.setupwizardlib;

import static com.google.common.truth.Truth.assertThat;

import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.android.car.setupwizardlib.robolectric.BaseRobolectricTest;
import com.android.car.setupwizardlib.robolectric.CarSetupWizardLibRobolectricTestRunner;
import com.android.car.setupwizardlib.robolectric.TestHelper;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;

import java.util.Locale;

/**
 * Tests for the CarSetupWizardLayout
 */
@RunWith(CarSetupWizardLibRobolectricTestRunner.class)
public class CarSetupWizardLayoutTest extends BaseRobolectricTest {
    private static final float COMPARISON_TOLERANCE = .01f;
    private static final Locale LOCALE_EN_US = new Locale("en", "US");
    // Hebrew locale can be used to test RTL.
    private static final Locale LOCALE_IW_IL = new Locale("iw", "IL");

    private CarSetupWizardLayout mCarSetupWizardLayout;
    private CarSetupWizardLayoutTestActivity mCarSetupWizardLayoutTestActivity;

    @Before
    public void setUp() {
        mCarSetupWizardLayoutTestActivity = Robolectric
                .buildActivity(CarSetupWizardLayoutTestActivity.class)
                .create()
                .get();

        mCarSetupWizardLayout = mCarSetupWizardLayoutTestActivity.
                findViewById(R.id.car_setup_wizard_layout);

        // Have to make this call first to ensure secondaryToolbar button is created from stub.
        mCarSetupWizardLayout.setSecondaryToolbarButtonVisible(true);
        mCarSetupWizardLayout.setSecondaryToolbarButtonVisible(false);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setBackButtonListener} does set the back button
     * listener.
     */
    @Test
    public void testSetBackButtonListener() {
        View.OnClickListener spyListener = TestHelper.createSpyListener();

        mCarSetupWizardLayout.setBackButtonListener(spyListener);
        mCarSetupWizardLayout.getBackButton().performClick();
        Mockito.verify(spyListener).onClick(mCarSetupWizardLayout.getBackButton());
    }

    /**
     * Test that {@link CarSetupWizardLayout#setBackButtonVisible} does set the view visible/not
     * visible and calls updateBackButtonTouchDelegate.
     */
    @Test
    public void testSetBackButtonVisibleTrue() {
        CarSetupWizardLayout spyCarSetupWizardLayout = Mockito.spy(mCarSetupWizardLayout);

        spyCarSetupWizardLayout.setBackButtonVisible(true);
        View backButton = spyCarSetupWizardLayout.getBackButton();
        TestHelper.assertViewVisible(backButton);
        Mockito.verify(spyCarSetupWizardLayout).updateBackButtonTouchDelegate(true);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setBackButtonVisible} does set the view visible/not
     * visible and calls updateBackButtonTouchDelegate.
     */
    @Test
    public void testSetBackButtonVisibleFalse() {
        CarSetupWizardLayout spyCarSetupWizardLayout = Mockito.spy(mCarSetupWizardLayout);

        spyCarSetupWizardLayout.setBackButtonVisible(false);
        View backButton = spyCarSetupWizardLayout.getBackButton();
        TestHelper.assertViewNotVisible(backButton);
        Mockito.verify(spyCarSetupWizardLayout).updateBackButtonTouchDelegate(false);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setToolbarTitleVisible} does set the view visible/not
     * visible.
     */
    @Test
    public void testSetToolbarTitleVisibleTrue() {
        View toolbarTitle = mCarSetupWizardLayout.getToolbarTitle();

        mCarSetupWizardLayout.setToolbarTitleVisible(true);
        TestHelper.assertViewVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setToolbarTitleVisible} does set the view visible/not
     * visible.
     */
    @Test
    public void testSetToolbarTitleVisibleFalse() {
        View toolbarTitle = mCarSetupWizardLayout.getToolbarTitle();

        mCarSetupWizardLayout.setToolbarTitleVisible(false);
        TestHelper.assertViewNotVisible(toolbarTitle);
    }

    /**
     * Tests that {@link CarSetupWizardLayout#setToolbarTitleText(String)} does set the toolbar
     * title text.
     */
    @Test
    public void testSetToolbarTitleText() {
        mCarSetupWizardLayout.setToolbarTitleText("test title");
        TestHelper.assertTextEqual(mCarSetupWizardLayout.getToolbarTitle(), "test title");
    }

    /**
     * Test that {@link CarSetupWizardLayout#setPrimaryToolbarButtonVisible} does set the view
     * visible/not visible.
     */
    @Test
    public void testSetPrimaryToolbarButtonVisibleTrue() {
        View toolbarTitle = mCarSetupWizardLayout.getPrimaryToolbarButton();

        mCarSetupWizardLayout.setPrimaryToolbarButtonVisible(true);
        TestHelper.assertViewVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setPrimaryToolbarButtonVisible} does set the view
     * visible/not visible.
     */
    @Test
    public void testSetPrimaryToolbarButtonVisibleFalse() {
        View toolbarTitle = mCarSetupWizardLayout.getPrimaryToolbarButton();

        mCarSetupWizardLayout.setPrimaryToolbarButtonVisible(false);
        TestHelper.assertViewNotVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setPrimaryToolbarButtonEnabled} does set the view
     * enabled/not enabled.
     */
    @Test
    public void testSetPrimaryToolbarButtonEnabledTrue() {
        View toolbarTitle = mCarSetupWizardLayout.getPrimaryToolbarButton();

        mCarSetupWizardLayout.setPrimaryToolbarButtonEnabled(true);
        TestHelper.assertViewEnabled(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setPrimaryToolbarButtonEnabled} does set the view
     * enabled/not enabled.
     */
    @Test
    public void testSetPrimaryToolbarButtonEnabledFalse() {
        View toolbarTitle = mCarSetupWizardLayout.getPrimaryToolbarButton();

        mCarSetupWizardLayout.setPrimaryToolbarButtonEnabled(false);
        TestHelper.assertViewNotEnabled(toolbarTitle);
    }

    /**
     * Tests that {@link CarSetupWizardLayout#setPrimaryToolbarButtonText(String)} does set the
     * primary toolbar button text.
     */
    @Test
    public void testSetPrimaryToolbarButtonText() {
        mCarSetupWizardLayout.setPrimaryToolbarButtonText("test title");
        TestHelper.assertTextEqual(mCarSetupWizardLayout.getPrimaryToolbarButton(), "test title");
    }

    /**
     * Test that {@link CarSetupWizardLayout#setPrimaryToolbarButtonListener} does set the primary
     * toolbar button listener.
     */
    @Test
    public void testSetPrimaryToolbarButtonListener() {
        View.OnClickListener spyListener = TestHelper.createSpyListener();

        mCarSetupWizardLayout.setPrimaryToolbarButtonListener(spyListener);
        mCarSetupWizardLayout.getPrimaryToolbarButton().performClick();
        Mockito.verify(spyListener).onClick(mCarSetupWizardLayout.getPrimaryToolbarButton());
    }

    /**
     * Test that {@link CarSetupWizardLayout#createPrimaryToolbarButton} creates a new button but
     * holds over the correct attributes.
     */
    @Test
    public void testCreatePrimaryButtonTrue() {
        Button currPrimaryToolbarButton = mCarSetupWizardLayout.getPrimaryToolbarButton();
        Button primaryToolbarButton = mCarSetupWizardLayout.createPrimaryToolbarButton(true);

        assertThat(primaryToolbarButton.getVisibility()).isEqualTo(
                currPrimaryToolbarButton.getVisibility());
        assertThat(primaryToolbarButton.isEnabled()).isEqualTo(
                currPrimaryToolbarButton.isEnabled());
        assertThat(primaryToolbarButton.getText()).isEqualTo(currPrimaryToolbarButton.getText());
        assertThat(primaryToolbarButton.getLayoutParams()).isEqualTo(
                currPrimaryToolbarButton.getLayoutParams());
    }

    /**
     * Test that {@link CarSetupWizardLayout#setSecondaryToolbarButtonVisible} does set the view
     * visible/not visible.
     */
    @Test
    public void testSetSecondaryToolbarButtonVisibleTrue() {
        View toolbarTitle = mCarSetupWizardLayout.getSecondaryToolbarButton();

        mCarSetupWizardLayout.setSecondaryToolbarButtonVisible(true);
        TestHelper.assertViewVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setSecondaryToolbarButtonVisible} does set the view
     * visible/not visible.
     */
    @Test
    public void testSetSecondaryToolbarButtonVisibleFalse() {
        View toolbarTitle = mCarSetupWizardLayout.getSecondaryToolbarButton();

        mCarSetupWizardLayout.setSecondaryToolbarButtonVisible(false);
        TestHelper.assertViewNotVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setSecondaryToolbarButtonEnabled} does set the view
     * enabled/not enabled.
     */
    @Test
    public void testSetSecondaryToolbarButtonEnabledTrue() {
        View toolbarTitle = mCarSetupWizardLayout.getSecondaryToolbarButton();

        mCarSetupWizardLayout.setSecondaryToolbarButtonEnabled(true);
        TestHelper.assertViewEnabled(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setSecondaryToolbarButtonEnabled} does set the view
     * enabled/not enabled.
     */
    @Test
    public void testSetSecondaryToolbarButtonEnabledFalse() {
        View toolbarTitle = mCarSetupWizardLayout.getSecondaryToolbarButton();

        mCarSetupWizardLayout.setSecondaryToolbarButtonEnabled(false);
        TestHelper.assertViewNotEnabled(toolbarTitle);
    }

    /**
     * Tests that {@link CarSetupWizardLayout#setSecondaryToolbarButtonText(String)} does set the
     * secondary toolbar button text.
     */
    @Test
    public void testSetSecondaryToolbarButtonText() {
        mCarSetupWizardLayout.setSecondaryToolbarButtonText("test title");
        TestHelper.assertTextEqual(mCarSetupWizardLayout.getSecondaryToolbarButton(), "test title");
    }

    /**
     * Test that {@link CarSetupWizardLayout#setSecondaryToolbarButtonListener} does set the
     * secondary toolbar button listener.
     */
    @Test
    public void testSetSecondaryToolbarButtonListener() {
        View.OnClickListener spyListener = TestHelper.createSpyListener();

        mCarSetupWizardLayout.setSecondaryToolbarButtonListener(spyListener);
        mCarSetupWizardLayout.getSecondaryToolbarButton().performClick();
        Mockito.verify(spyListener).onClick(mCarSetupWizardLayout.getSecondaryToolbarButton());
    }

    /**
     * Test that {@link CarSetupWizardLayout#setProgressBarVisible} does set the view
     * visible/not visible.
     */
    @Test
    public void testSetProgressBarVisibleTrue() {
        View toolbarTitle = mCarSetupWizardLayout.getProgressBar();

        mCarSetupWizardLayout.setProgressBarVisible(true);
        TestHelper.assertViewVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setProgressBarVisible} does set the view
     * visible/not visible.
     */
    @Test
    public void testSetProgressBarVisibleFalse() {
        View toolbarTitle = mCarSetupWizardLayout.getProgressBar();

        mCarSetupWizardLayout.setProgressBarVisible(false);
        TestHelper.assertViewNotVisible(toolbarTitle);
    }

    /**
     * Test that {@link CarSetupWizardLayout#setProgressBarIndeterminate(boolean)}
     * does set the progress bar intermediate/not indeterminate.
     */
    @Test
    public void testSetProgressBarIndeterminateTrue() {
        mCarSetupWizardLayout.setProgressBarIndeterminate(true);
        assertThat(mCarSetupWizardLayout.getProgressBar().isIndeterminate()).isTrue();
    }

    /**
     * Test that {@link CarSetupWizardLayout#setProgressBarIndeterminate(boolean)}
     * does set the progress bar intermediate/not indeterminate.
     */
    @Test
    public void testSetProgressBarIndeterminateFalse() {
        mCarSetupWizardLayout.setProgressBarIndeterminate(false);
        assertThat(mCarSetupWizardLayout.getProgressBar().isIndeterminate()).isFalse();
    }

    /**
     * Test that {@link CarSetupWizardLayout#setProgressBarProgress} does set the progress.
     */
    @Test
    public void testSetProgressBarProgress() {
        mCarSetupWizardLayout.setProgressBarProgress(80);
        assertThat(mCarSetupWizardLayout.getProgressBar().getProgress()).isEqualTo(80);
    }

    @Test
    public void testApplyUpdatedLocale() {
        mCarSetupWizardLayout.applyLocale(LOCALE_IW_IL);
        TextView toolbarTitle = mCarSetupWizardLayout.getToolbarTitle();
        Button primaryToolbarButton = mCarSetupWizardLayout.getPrimaryToolbarButton();
        Button secondaryToolbarButton = mCarSetupWizardLayout.getSecondaryToolbarButton();

        assertThat(toolbarTitle.getTextLocale()).isEqualTo(LOCALE_IW_IL);
        assertThat(primaryToolbarButton.getTextLocale()).isEqualTo(LOCALE_IW_IL);
        assertThat(secondaryToolbarButton.getTextLocale()).isEqualTo(LOCALE_IW_IL);

        mCarSetupWizardLayout.applyLocale(LOCALE_EN_US);
        assertThat(toolbarTitle.getTextLocale()).isEqualTo(LOCALE_EN_US);
        assertThat(primaryToolbarButton.getTextLocale()).isEqualTo(LOCALE_EN_US);
        assertThat(secondaryToolbarButton.getTextLocale()).isEqualTo(LOCALE_EN_US);
    }

    @Test
    public void testGetBackButton() {
        assertThat(mCarSetupWizardLayout.getPrimaryToolbarButton()).isEqualTo(
                mCarSetupWizardLayout.findViewById(R.id.primary_toolbar_button));
    }

    @Test
    public void testGetToolBarTitle() {
        assertThat(mCarSetupWizardLayout.getToolbarTitle()).isEqualTo(
                mCarSetupWizardLayout.findViewById(R.id.toolbar_title));
    }

    @Test
    public void testGetPrimaryToolBarButton() {
        assertThat(mCarSetupWizardLayout.getPrimaryToolbarButton()).isEqualTo(
                mCarSetupWizardLayout.findViewById(R.id.primary_toolbar_button));
    }

    @Test
    public void testGetSecondaryToolBarButton() {
        assertThat(mCarSetupWizardLayout.getSecondaryToolbarButton()).isEqualTo(
                mCarSetupWizardLayout.findViewById(R.id.secondary_toolbar_button));
    }

    @Test
    public void testGetProgressBar() {
        assertThat(mCarSetupWizardLayout.getProgressBar()).isEqualTo(
                mCarSetupWizardLayout.findViewById(R.id.progress_bar));
    }

    @Test
    public void testTitleBarElevationChange() {
        mCarSetupWizardLayout.addElevationToTitleBar(/*animate= */ false);
        View titleBar = mCarSetupWizardLayout.findViewById(R.id.application_bar);
        assertThat(titleBar.getElevation()).isWithin(COMPARISON_TOLERANCE).of(
                RuntimeEnvironment.application.getResources().getDimension(
                        R.dimen.title_bar_drop_shadow_elevation));

        mCarSetupWizardLayout.removeElevationFromTitleBar(/*animate= */ false);
        assertThat(titleBar.getElevation()).isWithin(COMPARISON_TOLERANCE).of(0f);
    }
}
