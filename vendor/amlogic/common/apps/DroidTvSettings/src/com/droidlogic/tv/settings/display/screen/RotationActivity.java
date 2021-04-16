package com.droidlogic.tv.settings.display.screen;

import android.app.Fragment;
import com.droidlogic.tv.settings.BaseSettingsFragment;
import com.droidlogic.tv.settings.TvSettingsActivity;


public class RotationActivity extends TvSettingsActivity{
    @Override
    protected Fragment createSettingsFragment() {
        return SettingsFragment.newInstance(); 
    }

    public static class SettingsFragment extends BaseSettingsFragment {

        public static SettingsFragment newInstance() {
             return new SettingsFragment();
        }

        @Override
        public void onPreferenceStartInitialScreen() {
            final RotationFragment fragment = RotationFragment.newInstance();
            startPreferenceFragment(fragment);
        }
    }
}
