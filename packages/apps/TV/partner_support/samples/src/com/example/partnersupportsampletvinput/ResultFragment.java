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

import android.app.Activity;
import android.app.FragmentManager;
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import com.google.android.tv.partner.support.EpgContract;
import java.util.List;

/** Result Fragment shows the setup result */
public class ResultFragment extends GuidedStepFragment {
    private static final long ACTION_ID_LINEUP = 1;

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new Guidance("ResultFragment", "ResultFragment Description", null, null);
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        String title = getArguments().getString(LineupSelectionFragment.KEY_TITLE);
        String description = getArguments().getString(LineupSelectionFragment.KEY_DESCRIPTION);
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(ACTION_ID_LINEUP)
                        .title("Selected Lineup")
                        .description(title + " (" + description + ")")
                        .focusable(false)
                        .build());
    }

    @Override
    public void onCreateButtonActions(
            @NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_FINISH)
                        .title("FINISH")
                        .build());
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_CANCEL)
                        .title("BACK")
                        .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (action == null) {
            return;
        }
        FragmentManager fm = getFragmentManager();
        switch ((int) action.getId()) {
            case (int) GuidedAction.ACTION_ID_CANCEL:
                super.onGuidedActionClicked(action);
                fm.popBackStack();
                break;
            case (int) GuidedAction.ACTION_ID_FINISH:
                setResultAndFinishActivity(SampleTvInputService.INPUT_ID);
                finishGuidedStepFragments();
                break;
            default:
        }
    }

    private void setResultAndFinishActivity(String inputId) {
        Intent data = new Intent();
        data.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
        data.putExtra(EpgContract.EXTRA_USE_CLOUD_EPG, true);
        getActivity().setResult(Activity.RESULT_OK, data);
        getActivity().finishActivity(SampleTvInputSetupActivity.REQUEST_CODE_START_SETUP_ACTIVITY);
    }
}
