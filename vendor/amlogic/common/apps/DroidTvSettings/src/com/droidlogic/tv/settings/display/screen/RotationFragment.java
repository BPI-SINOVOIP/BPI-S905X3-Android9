/* Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.droidlogic.tv.settings.display.screen;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.Looper;
import android.os.Build;
import android.provider.Settings;
import android.provider.Settings.System;
import android.support.v14.preference.SwitchPreference;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.content.ContentResolver;
import android.database.ContentObserver;
import android.view.Display;
import android.util.ArrayMap;
import android.util.Log;

import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class RotationFragment extends LeanbackPreferenceFragment {
	private static final String TAG = "RotationFragment";

	private static final String ROTATION_RADIO_GROUP = "rotation";
	private static final String ACTION_ROTATION_0 = "0";
	private static final String ACTION_ROTATION_90 = "90";
	private static final String ACTION_ROTATION_180 = "180";
	private static final String ACTION_ROTATION_270 = "270";
        private static final String KEY_AUTO_ROTATE = "auto-rotate";
        private static final String KEY_APP_FORCE_LAND = "force_land";


	private SwitchPreference mAutoRotatePre;
	private SwitchPreference mAppForceLandPre;
	private ContentResolver mContentResolver;
	private Context mContext;
	private static int degree;

	private ContentObserver mAutoRotationObserver =
		new ContentObserver(new Handler(Looper.getMainLooper())) {
		@Override
		public void onChange(boolean selfChange) {
			updateAutoRotation();
		}
	};


	public static RotationFragment newInstance() {
		return new RotationFragment();
	}

	private boolean hasAutoRotate() {
		if (Build.MODEL.equals("VIM3") || Build.MODEL.equals("VIM3L"))
			return true;
		else
			return false;
	}

	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
		Display display = getActivity().getWindowManager().getDefaultDisplay();
		degree = display.getRotation();

		mContext = getPreferenceManager().getContext();
		final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(mContext);
		screen.setTitle(R.string.screen_rotation);
		Preference activePref = null;

                mAppForceLandPre = new SwitchPreference(getPreferenceManager().getContext());
                mAppForceLandPre.setTitle("Force land for application");
                mAppForceLandPre.setKey(KEY_APP_FORCE_LAND);
                screen.addPreference(mAppForceLandPre);

                if (hasAutoRotate()) {
                    mAutoRotatePre = new SwitchPreference(getPreferenceManager().getContext());
                    mAutoRotatePre.setTitle("Auto-rotate");
                    mAutoRotatePre.setKey(KEY_AUTO_ROTATE);
                    screen.addPreference(mAutoRotatePre);
                }
		final List<Action> InfoList = getActions();
		for (final Action info : InfoList) {
			final String tag = info.getKey();
			final RadioPreference radioPreference = new RadioPreference(mContext);
			radioPreference.setKey(tag);
			radioPreference.setPersistent(false);
			radioPreference.setTitle(info.getTitle());
			radioPreference.setRadioGroup(ROTATION_RADIO_GROUP);
			radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

			if (info.isChecked()) {
				radioPreference.setChecked(true);
				activePref = radioPreference;
			}

			screen.addPreference(radioPreference);
		}
		if (activePref != null && savedInstanceState == null) {
			scrollToPreference(activePref);
		}

		setPreferenceScreen(screen);
		mContentResolver = mContext.getContentResolver();
	}

	@Override
	public void onResume() {
		super.onResume();
		updateAutoRotation();
		mContentResolver.registerContentObserver(System.getUriFor(Settings.System.ACCELEROMETER_ROTATION), false, mAutoRotationObserver);
	}

	@Override
	public void onPause() {
		super.onPause();
		mContentResolver.unregisterContentObserver(mAutoRotationObserver);
	}

	private ArrayList<Action> getActions() {
		ArrayList<Action> actions = new ArrayList<Action>();
		actions.add(new Action.Builder().key(ACTION_ROTATION_0).title(getString(R.string.screen_rotation_0))
				.checked(degree == 0).build());
		actions.add(new Action.Builder().key(ACTION_ROTATION_90).title(getString(R.string.screen_rotation_90))
				.checked(degree == 1).build());
		actions.add(new Action.Builder().key(ACTION_ROTATION_180).title(getString(R.string.screen_rotation_180))
				.checked(degree == 2).build());
		actions.add(new Action.Builder().key(ACTION_ROTATION_270).title(getString(R.string.screen_rotation_270))
				.checked(degree == 3).build());
		return actions;
	}

	private void setRotation(int val) {
		android.provider.Settings.System.putInt(mContext.getContentResolver(), Settings.System.USER_ROTATION, val);
		SystemProperties.set("persist.sys.rotation", String.valueOf(val));
		degree = val;
	}

	private void setAutoRotation(int enabled) {
		android.provider.Settings.System.putInt(mContext.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, enabled);
        }

	private int getAutoRotaion() {
		return  android.provider.Settings.System.getInt(mContext.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 0);
	}

	private void updateAutoRotation() {
                if (SystemProperties.get("persist.sys.app.rotation", "original").equals("force_land")) {
                    updateForceLand(true);
                    return;
                }
		boolean enabled = getAutoRotaion() == 1 ? true:false;
		if (mAutoRotatePre != null) {
			mAutoRotatePre.setChecked(enabled);
		}
		updateRotation(enabled);
	}

	private void updateRotation(boolean isAuto) {
		final List<Action> InfoList = getActions();
		for (final Action info : InfoList) {
			final String tag = info.getKey();
			RadioPreference radio = (RadioPreference) findPreference(tag);
			radio.setEnabled(!isAuto);
		}
	}


        private void updateForceLand(boolean force) {
                if (force) {
                     SystemProperties.set("persist.sys.app.rotation", "force_land");
                     setRotation(0);
                     setAutoRotation(0);
                     if (mAutoRotatePre != null) {
                         mAutoRotatePre.setChecked(false);
                         mAutoRotatePre.setEnabled(false);
                     }
                     updateRotation(true);
                } else {
                     SystemProperties.set("persist.sys.app.rotation", "original");
                     updateRotation(false);
                     if (mAutoRotatePre != null) {
                         mAutoRotatePre.setEnabled(true);
                     }
                }
        }

	@Override
	public boolean onPreferenceTreeClick(Preference preference) {
		if (preference instanceof RadioPreference) {
			final RadioPreference radioPreference = (RadioPreference) preference;
			radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
			if (radioPreference.isChecked()) {
				String key = radioPreference.getKey().toString();
				if (key.equals(ACTION_ROTATION_0)) {
					setRotation(0);
				}
				if (key.equals(ACTION_ROTATION_90)) {
					setRotation(1);
				}
				if (key.equals(ACTION_ROTATION_180)) {
					setRotation(2);
				}
				if (key.equals(ACTION_ROTATION_270)) {
					setRotation(3);
				}
			} else {
				radioPreference.setChecked(true);
			}
			return super.onPreferenceTreeClick(preference);
		}
		if (preference.getKey().equals(KEY_AUTO_ROTATE)) {
			final SwitchPreference pref = (SwitchPreference) preference;
			boolean enabled = pref.isChecked();
			setAutoRotation(enabled?1:0);
			updateRotation(enabled);
		}
                if (preference.getKey().equals(KEY_APP_FORCE_LAND)) {
                        final SwitchPreference pref = (SwitchPreference) preference;
                        boolean enabled = pref.isChecked();
                        updateForceLand(enabled);
                }

		return super.onPreferenceTreeClick(preference);
	}
}
