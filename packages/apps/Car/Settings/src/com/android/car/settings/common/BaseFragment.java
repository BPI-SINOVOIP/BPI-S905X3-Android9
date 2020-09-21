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
 * limitations under the License
 */

package com.android.car.settings.common;

import android.annotation.NonNull;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.LayoutRes;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.car.settings.R;

import java.util.Set;

/**
 * Base fragment for setting activity.
 */
public abstract class BaseFragment extends Fragment {
    public static final String EXTRA_TITLE_ID = "extra_title_id";
    public static final String EXTRA_LAYOUT = "extra_layout";
    public static final String EXTRA_ACTION_BAR_LAYOUT = "extra_action_bar_layout";
    /**
     * For indicating a fragment is running in Setup Wizard
     */
    public static final String EXTRA_RUNNING_IN_SETUP_WIZARD = "extra_running_in_setup_wizard";

    /**
     * Controls the transition of fragment.
     */
    public interface FragmentController {
        /**
         * Launches fragment in the main container of current activity.
         */
        void launchFragment(BaseFragment fragment);

        /**
         * Pops the top off the fragment stack.
         * @return {@code false} if there's no stack to pop, {@code true} otherwise
         */
        void goBack();

        /**
         * Shows a message that current feature is not available when driving.
         */
        void showDOBlockingMessage();
    }

    /**
     * Provides current CarUxRestrictions.
     */
    public interface UXRestrictionsProvider {

        /**
         * Fetches current CarUxRestrictions
         */
        @NonNull
        CarUxRestrictions getCarUxRestrictions();
    }

    @LayoutRes
    protected int mLayout;

    @LayoutRes
    private int mActionBarLayout;

    @StringRes
    private int mTitleId;

    /**
     * Assume The activity holds this fragment also implements the FragmentController.
     * This function should be called after onAttach()
     */
    public final FragmentController getFragmentController() {
        return (FragmentController) getActivity();
    }

    /**
     * Assume The activity holds this fragment also implements the UXRestrictionsProvider.
     * This function should be called after onAttach()
     */
    protected final CarUxRestrictions getCurrentRestrictions() {
        return ((UXRestrictionsProvider) getActivity()).getCarUxRestrictions();
    }

    protected static Bundle getBundle() {
        Bundle bundle = new Bundle();
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar);
        return bundle;
    }

    /**
     * Checks if this fragment can be shown or not given the CarUxRestrictions. Default to
     * {@code false} if UX_RESTRICTIONS_NO_SETUP is set.
     */
    protected boolean canBeShown(@NonNull CarUxRestrictions carUxRestrictions) {
        return !CarUxRestrictionsHelper.isNoSetup(carUxRestrictions);
    }

    /**
     * Notifies the fragment with the latest CarUxRestrictions change.
     */
    protected void onUxRestrictionChanged(@NonNull CarUxRestrictions carUxRestrictions) {
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (!(getActivity() instanceof FragmentController)) {
            throw new IllegalArgumentException("Must attach to an FragmentController");
        }
        if (!(getActivity() instanceof UXRestrictionsProvider)) {
            throw new IllegalArgumentException("Must attach to an UXRestrictionsProvider");
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Set<String> extraKeys = getArguments().keySet();
        if (extraKeys.contains(EXTRA_ACTION_BAR_LAYOUT)) {
            mActionBarLayout = getArguments().getInt(EXTRA_ACTION_BAR_LAYOUT);
        } else {
            throw new IllegalArgumentException("must specify a actionBar layout");
        }
        if (extraKeys.contains(EXTRA_LAYOUT)) {
            mLayout = getArguments().getInt(EXTRA_LAYOUT);
        } else {
            throw new IllegalArgumentException("must specify a layout");
        }
        if (extraKeys.contains(EXTRA_TITLE_ID)) {
            mTitleId = getArguments().getInt(EXTRA_TITLE_ID);
        } else {
            throw new IllegalArgumentException("must specify a title");
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        onUxRestrictionChanged(getCurrentRestrictions());
    }

    /**
     * Should be used to override fragment's title.
     * Should be called after {@code super.onActivityCreated}, so that it's called AFTER the default title
     * setter.
     *
     * @param title CharSequence to set as the new title.
     */
    protected final void setTitle(CharSequence title) {
        TextView titleView = getActivity().findViewById(R.id.title);
        titleView.setText(title);
    }

    /**
     * Allow fragment to intercept back press and customize behavior.
     */
    protected void onBackPressed() {
        getFragmentController().goBack();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(mLayout, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        ActionBar actionBar = ((AppCompatActivity) getActivity()).getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(false);
        actionBar.setCustomView(mActionBarLayout);
        actionBar.setDisplayShowCustomEnabled(true);
        // make the toolbar take the whole width.
        Toolbar toolbar = (Toolbar) actionBar.getCustomView().getParent();
        toolbar.setPadding(0, 0, 0, 0);
        getActivity().findViewById(R.id.action_bar_icon_container).setOnClickListener(
                v -> onBackPressed());
        TextView titleView = getActivity().findViewById(R.id.title);
        titleView.setText(mTitleId);
    }
}
