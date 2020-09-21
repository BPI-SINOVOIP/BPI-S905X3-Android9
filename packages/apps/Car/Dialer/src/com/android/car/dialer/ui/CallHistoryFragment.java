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
package com.android.car.dialer.ui;

import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.ViewModelProviders;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.car.widget.ListItemAdapter;
import androidx.car.widget.PagedListView;

import com.android.car.dialer.R;
import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.ui.viewmodel.CallHistoryViewModel;

import java.util.List;

public class CallHistoryFragment extends Fragment {
    public static final String CALL_TYPE_KEY = "CALL_TYPE_KEY";

    public static CallHistoryFragment newInstance(@PhoneLoader.CallType int callType) {
        CallHistoryFragment fragment = new CallHistoryFragment();
        Bundle arg = new Bundle();
        arg.putInt(CALL_TYPE_KEY, callType);
        fragment.setArguments(arg);
        return fragment;
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.call_list_fragment, container, false);
        PagedListView pagedListView = fragmentView.findViewById(R.id.list_view);
        CallHistoryListItemProvider callHistoryListItemProvider = new CallHistoryListItemProvider();
        ListItemAdapter adapter = new ListItemAdapter(getContext(), callHistoryListItemProvider);
        pagedListView.setAdapter(adapter);

        int callType = getArguments().getInt(CALL_TYPE_KEY);

        CallHistoryViewModel viewModel = ViewModelProviders.of(this).get(
                CallHistoryViewModel.class);

        LiveData<List<CallLogListingTask.CallLogItem>> liveData = null;
        if (callType == PhoneLoader.CallType.CALL_TYPE_ALL) {
            liveData = viewModel.getCallHistory();
        } else if (callType == PhoneLoader.CallType.MISSED_TYPE) {
            liveData = viewModel.getMissedCallHistory();
        }

        if (liveData != null) {
            liveData.observe(this,
                    callHistoryItems -> {
                        callHistoryListItemProvider.setCallHistoryListItems(getContext(),
                                callHistoryItems);
                        adapter.notifyDataSetChanged();
                    });
        }

        return fragmentView;
    }
}
