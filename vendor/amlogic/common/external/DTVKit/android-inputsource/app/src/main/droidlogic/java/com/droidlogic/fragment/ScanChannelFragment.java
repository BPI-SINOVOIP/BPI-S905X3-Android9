package com.droidlogic.fragment;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.dtvkit.inputsource.R;

public class ScanChannelFragment extends Fragment {

    public static ScanChannelFragment newInstance() {
        return new ScanChannelFragment();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_scan_channel, container, false);
        return rootView;
    }
}
