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

package com.example.partnersupportsampletvinput;

import android.app.FragmentManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import java.util.List;

/** Location Fragment for users to enter postal code */
public class LocationFragment extends GuidedStepFragment {
    public static final long ACTION_ID_ZIPCODE = 1;
    private final GuidedAction mZipCodeAction =
            new GuidedAction.Builder(getActivity())
                    .id(ACTION_ID_ZIPCODE)
                    .title("zip code")
                    .description("please enter zip code")
                    .editDescription("")
                    .descriptionEditable(true)
                    .build();

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new Guidance("LocationFragment", "LocationFragment Description", null, null);
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(mZipCodeAction);
    }

    @Override
    public void onCreateButtonActions(
            @NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_NEXT)
                        .title("NEXT")
                        .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (action == null) {
            return;
        }
        FragmentManager fm = getFragmentManager();
        switch ((int) action.getId()) {
            case (int) ACTION_ID_ZIPCODE:
                super.onGuidedActionClicked(action);
                break;
            case (int) GuidedAction.ACTION_ID_NEXT:
                ChannelScanFragment fragment = new ChannelScanFragment();
                Bundle args = new Bundle();
                // TODO: validate the input postcode
                args.putString(
                        ChannelScanFragment.KEY_POSTCODE,
                        mZipCodeAction.getDescription().toString());
                fragment.setArguments(args);
                GuidedStepFragment.add(fm, fragment);
                break;
            default:
        }
    }

    @Override
    public long onGuidedActionEditedAndProceed(GuidedAction action) {
        long result = super.onGuidedActionEditedAndProceed(action);
        action.setDescription(action.getEditDescription());
        return result;
    }
}
