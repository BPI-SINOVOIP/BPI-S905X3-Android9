/* Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.droidlogic.tv.settings.display.sdr;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.util.ArrayMap;
import android.util.Log;

import com.droidlogic.app.SdrManager;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class SdrSettingFragment extends LeanbackPreferenceFragment {
	private static final String TAG = "SdrSettingFragment";

	private static final String SDR_RADIO_GROUP = "sdr";
	private static final String ACTION_AUTO = "auto";
	private static final String ACTION_ON = "on";
	private static final String ACTION_OFF = "off";
	private SdrManager mSdrManager;

	// Adjust this value to keep things relatively responsive without janking
	// animations
	private static final int SDR_SET_DELAY_MS = 500;
	private final Handler mDelayHandler = new Handler();
	private String mNewSdrMode;
	private final Runnable mSetSdrRunnable = new Runnable() {
		@Override
		public void run() {
			if (ACTION_AUTO.equals(mNewSdrMode)) {
				mSdrManager.setSdrMode(SdrManager.MODE_AUTO);
			} else if (ACTION_OFF.equals(mNewSdrMode)) {
				mSdrManager.setSdrMode(SdrManager.MODE_OFF);
			}
		}
	};

	public static SdrSettingFragment newInstance() {
		return new SdrSettingFragment();
	}

	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
		mSdrManager = new SdrManager((Context) getActivity());
		final Context themedContext = getPreferenceManager().getContext();
		final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
		screen.setTitle(R.string.device_sdr);
		String currentSdrMode = null;
		Preference activePref = null;

		final List<Action> sdrInfoList = getActions();
		for (final Action sdrInfo : sdrInfoList) {
			final String sdrTag = sdrInfo.getKey();
			final RadioPreference radioPreference = new RadioPreference(themedContext);
			radioPreference.setKey(sdrTag);
			radioPreference.setPersistent(false);
			radioPreference.setTitle(sdrInfo.getTitle());
			radioPreference.setRadioGroup(SDR_RADIO_GROUP);
			radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

			if (sdrInfo.isChecked()) {
				currentSdrMode = sdrTag;
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

	private ArrayList<Action> getActions() {
		int mode = mSdrManager.getSdrMode();
		ArrayList<Action> actions = new ArrayList<Action>();
		actions.add(new Action.Builder().key(ACTION_AUTO).title(getString(R.string.on))
				.checked(mode == SdrManager.MODE_AUTO).build());
		actions.add(new Action.Builder().key(ACTION_OFF).title(getString(R.string.off))
				.checked(mode == SdrManager.MODE_OFF).build());
		return actions;
	}

	@Override
	public boolean onPreferenceTreeClick(Preference preference) {
		if (preference instanceof RadioPreference) {
			final RadioPreference radioPreference = (RadioPreference) preference;
			radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
			if (radioPreference.isChecked()) {
				mNewSdrMode = radioPreference.getKey().toString();
				mDelayHandler.removeCallbacks(mSetSdrRunnable);
				mDelayHandler.postDelayed(mSetSdrRunnable, SDR_SET_DELAY_MS);
			} else {
				radioPreference.setChecked(true);
			}
		}
		return super.onPreferenceTreeClick(preference);
	}
}
