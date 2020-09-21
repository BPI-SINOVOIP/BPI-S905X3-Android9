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

/** Welcome Fragment shows welcome information for users */
public class WelcomeFragment extends GuidedStepFragment {

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new Guidance("WelcomeFragment Title", "WelcomeFragment Description", null, null);
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_NEXT)
                        .title("NEXT")
                        .build());
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_CANCEL)
                        .title("CANCEL")
                        .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (action == null) {
            return;
        }
        FragmentManager fm = getFragmentManager();
        switch ((int) action.getId()) {
            case (int) GuidedAction.ACTION_ID_NEXT:
                GuidedStepFragment.add(fm, new LocationFragment());
                break;
            case (int) GuidedAction.ACTION_ID_CANCEL:
                finishGuidedStepFragments();
                break;
            default:
        }
    }
}
