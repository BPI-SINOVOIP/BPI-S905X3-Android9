package com.droidlogic.fragment;

import android.app.Fragment;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.Toast;

import org.dtvkit.inputsource.R;

public class PlaceholderFragment extends Fragment {

    private final static String TAG = "PlaceholderFragment";
    private Button mDishSetup;
    private Button mScanChannel;

    public PlaceholderFragment() {
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_main, container, false);
        initButton(rootView);
        return rootView;
    }

    private void initButton(final View view) {
        if (view == null) {
            return;
        }
        mDishSetup = (Button) view.findViewById(R.id.button_dish_setup);
        mScanChannel = (Button) view.findViewById(R.id.button_scan_channel);
        mScanChannel.requestFocus();
        mDishSetup.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //getFragmentManager().beginTransaction().add(R.id.container, DishSetupFragment.newInstance()).commit();
                ((ScanMainActivity)getActivity()).getScanFragmentManager().show(new ScanDishSetupFragment());
                Log.d(TAG, "start ScanDishSetupFragment");
            }
        });
        mScanChannel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //getFragmentManager().beginTransaction().add(R.id.container, ScanChannelFragment.newInstance()).commit();
                    /*ScanChannelFragment scanfragment = ScanChannelFragment.newInstance();
                    scanfragment.setArguments(getIntent().getExtras());
                    mScanFragmentManager.show(scanfragment);
                    mCurrentFragment = SCAN_FRAGMENT;*/
                try {
                    Intent intent = getActivity().getIntent();
                    //intent.setClassName("com.android.tv", "com.android.tv.droidlogic.ChannelSearchActivity");
                    intent.setClassName("org.dtvkit.inputsource", "org.dtvkit.inputsource.DtvkitDvbsSetup");
                    getActivity().startActivityForResult(intent, ScanMainActivity.REQUEST_CODE_START_SETUP_ACTIVITY);
                    Log.d(TAG, "ScanChannelFragment");
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(getActivity(), getString(R.string.search_activity_not_found), Toast.LENGTH_SHORT).show();
                    return;
                }
            }
        });
    }
}
