package com.android.car.dialer.ui;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.android.car.apps.common.FabDrawable;
import com.android.car.dialer.R;
import com.android.car.dialer.telecom.UiCall;
import com.android.car.dialer.telecom.UiCallManager;

public class RingingCallControllerBarFragment extends Fragment {

    public static RingingCallControllerBarFragment newInstance() {
        return new RingingCallControllerBarFragment();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.ringing_call_controller_bar_fragment,
                container, false);

        fragmentView.findViewById(R.id.answer_call_button).setOnClickListener((v) -> answerCall());
        fragmentView.findViewById(R.id.answer_call_text).setOnClickListener((v) -> answerCall());
        fragmentView.findViewById(R.id.end_call_button).setOnClickListener((v) -> declineCall());
        fragmentView.findViewById(R.id.end_call_text).setOnClickListener((v) -> declineCall());

        return fragmentView;
    }

    private void answerCall() {
        UiCallManager uiCallManager = UiCallManager.get();
        UiCall primaryCall = uiCallManager.getPrimaryCall();
        uiCallManager.answerCall(primaryCall);
    }

    private void declineCall() {
        UiCallManager uiCallManager = UiCallManager.get();
        UiCall primaryCall = uiCallManager.getPrimaryCall();
        uiCallManager.rejectCall(primaryCall, false, null);
    }
}
