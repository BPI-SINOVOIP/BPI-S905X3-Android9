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
import android.os.ServiceManager;
import android.os.UserHandle;
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
import android.view.IWindowManager;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class DensityFragment extends LeanbackPreferenceFragment {
	private static final String TAG = "DensityFragment";

	private static final String DENSITY_RADIO_GROUP = "density";
	private static final String ACTION_DENSITY_120 = "120";
	private static final String ACTION_DENSITY_160 = "160";
	private static final String ACTION_DENSITY_200 = "200";
	private static final String ACTION_DENSITY_240 = "240";
	private static final String ACTION_DENSITY_280 = "280";
	private static final String ACTION_DENSITY_320 = "320";
	private static final int DEFAULT_DISPLAY = 0;


	private Context mContext;
	private static int density;
	protected IWindowManager mWm;



	public static DensityFragment newInstance() {
		return new DensityFragment();
	}


	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

		mWm = IWindowManager.Stub.asInterface(ServiceManager.checkService(Context.WINDOW_SERVICE));
		density = getDensity();
		mContext = getPreferenceManager().getContext();
		final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(mContext);
		screen.setTitle(R.string.screen_density);
		Preference activePref = null;

		final List<Action> InfoList = getActions();
		for (final Action info : InfoList) {
			final String tag = info.getKey();
			final RadioPreference radioPreference = new RadioPreference(mContext);
			radioPreference.setKey(tag);
			radioPreference.setPersistent(false);
			radioPreference.setTitle(info.getTitle());
			radioPreference.setRadioGroup(DENSITY_RADIO_GROUP);
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
	}

	@Override
	public void onResume() {
		super.onResume();
	}

	@Override
	public void onPause() {
		super.onPause();
	}

	private ArrayList<Action> getActions() {
		ArrayList<Action> actions = new ArrayList<Action>();
		actions.add(new Action.Builder().key(ACTION_DENSITY_120).title(getString(R.string.screen_density_120))
				.checked(density == 120).build());
		actions.add(new Action.Builder().key(ACTION_DENSITY_160).title(getString(R.string.screen_density_160))
				.checked(density == 160).build());
		actions.add(new Action.Builder().key(ACTION_DENSITY_200).title(getString(R.string.screen_density_200))
				.checked(density == 200).build());
		actions.add(new Action.Builder().key(ACTION_DENSITY_240).title(getString(R.string.screen_density_240))
				.checked(density == 240).build());
		actions.add(new Action.Builder().key(ACTION_DENSITY_280).title(getString(R.string.screen_density_280))
				.checked(density == 280).build());
		actions.add(new Action.Builder().key(ACTION_DENSITY_320).title(getString(R.string.screen_density_320))
				.checked(density == 320).build());
		return actions;
	}

	private void setDensity(int density) {
		try {
			mWm.setForcedDisplayDensityForUser(DEFAULT_DISPLAY, density, UserHandle.USER_CURRENT);
		} catch (RemoteException e) {
		}
	}

	private int getDensity() {
		try {
			int initialDensity = mWm.getInitialDisplayDensity(DEFAULT_DISPLAY);
			int baseDensity = mWm.getBaseDisplayDensity(DEFAULT_DISPLAY);
			if (initialDensity != baseDensity) {
				return baseDensity;
			} else {
				return initialDensity;
			}
		} catch (RemoteException e) {
		}
		return 0;
	}

	@Override
	public boolean onPreferenceTreeClick(Preference preference) {
		if (preference instanceof RadioPreference) {
			final RadioPreference radioPreference = (RadioPreference) preference;
			radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
			if (radioPreference.isChecked()) {
				String key = radioPreference.getKey().toString();
				if (key.equals(ACTION_DENSITY_120)) {
					setDensity(120);
				}
				if (key.equals(ACTION_DENSITY_160)) {
					setDensity(160);
				}
				if (key.equals(ACTION_DENSITY_200)) {
					setDensity(200);
				}
				if (key.equals(ACTION_DENSITY_240)) {
					setDensity(240);
				}
				if (key.equals(ACTION_DENSITY_280)) {
					setDensity(280);
				}
				if (key.equals(ACTION_DENSITY_320)) {
					setDensity(320);
				}
			} else {
				radioPreference.setChecked(true);
			}
			return super.onPreferenceTreeClick(preference);
		}
		return super.onPreferenceTreeClick(preference);
	}
}
