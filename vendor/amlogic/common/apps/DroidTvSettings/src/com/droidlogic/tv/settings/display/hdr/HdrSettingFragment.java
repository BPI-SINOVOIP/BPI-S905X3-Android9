/* Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.droidlogic.tv.settings.display.hdr;

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

import com.droidlogic.app.HdrManager;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class HdrSettingFragment extends LeanbackPreferenceFragment {
	private static final String TAG = "HdrSettingFragment";

	private static final String HDR_RADIO_GROUP = "hdr";
	private static final String ACTION_AUTO = "auto";
	private static final String ACTION_ON = "on";
	private static final String ACTION_OFF = "off";
	private HdrManager mHdrManager;

	// Adjust this value to keep things relatively responsive without janking
	// animations
	private static final int HDR_SET_DELAY_MS = 500;
	private final Handler mDelayHandler = new Handler();
	private String mNewHdrMode;
	private final Runnable mSetHdrRunnable = new Runnable() {
		@Override
		public void run() {
			if (ACTION_AUTO.equals(mNewHdrMode)) {
				mHdrManager.setHdrMode(HdrManager.MODE_AUTO);
			} else if (ACTION_ON.equals(mNewHdrMode)) {
				mHdrManager.setHdrMode(HdrManager.MODE_ON);
			} else if (ACTION_OFF.equals(mNewHdrMode)) {
				mHdrManager.setHdrMode(HdrManager.MODE_OFF);
			}
		}
	};

	public static HdrSettingFragment newInstance() {
		return new HdrSettingFragment();
	}

	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
		mHdrManager = new HdrManager((Context) getActivity());
		final Context themedContext = getPreferenceManager().getContext();
		final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
		screen.setTitle(R.string.device_hdr);
		String currentHdrMode = null;
		Preference activePref = null;

		final List<Action> hdrInfoList = getActions();
		for (final Action hdrInfo : hdrInfoList) {
			final String hdrTag = hdrInfo.getKey();
			final RadioPreference radioPreference = new RadioPreference(themedContext);
			radioPreference.setKey(hdrTag);
			radioPreference.setPersistent(false);
			radioPreference.setTitle(hdrInfo.getTitle());
			radioPreference.setRadioGroup(HDR_RADIO_GROUP);
			radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

			if (hdrInfo.isChecked()) {
				currentHdrMode = hdrTag;
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
		int mode = mHdrManager.getHdrMode();
		ArrayList<Action> actions = new ArrayList<Action>();
		actions.add(new Action.Builder().key(ACTION_AUTO).title(getString(R.string.device_sound_dts_trans_auto))
				.checked(mode == HdrManager.MODE_AUTO).build());
		actions.add(new Action.Builder().key(ACTION_ON).title(getString(R.string.on))
				.checked(mode == HdrManager.MODE_ON).build());
		actions.add(new Action.Builder().key(ACTION_OFF).title(getString(R.string.off))
				.checked(mode == HdrManager.MODE_OFF).build());
		return actions;
	}

	@Override
	public boolean onPreferenceTreeClick(Preference preference) {
		if (preference instanceof RadioPreference) {
			final RadioPreference radioPreference = (RadioPreference) preference;
			radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
			if (radioPreference.isChecked()) {
				mNewHdrMode = radioPreference.getKey().toString();
				mDelayHandler.removeCallbacks(mSetHdrRunnable);
				mDelayHandler.postDelayed(mSetHdrRunnable, HDR_SET_DELAY_MS);
			} else {
				radioPreference.setChecked(true);
			}
		}
		return super.onPreferenceTreeClick(preference);
	}
}
