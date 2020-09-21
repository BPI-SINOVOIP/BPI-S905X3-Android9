/* Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.droidlogic.tv.settings.display.dolbyvision;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.TwoStatePreference;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;

import com.droidlogic.app.DolbyVisionSettingManager;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class GraphicsPrioritySettingFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "GraphicsPrioritySettingFragment";

    //public static final String KEY_DOLBY_VISION     = "dolby_vision_set";

    private static final String GRAPHICS_PRIORITY      = "1";
    private static final String VIDEO_PRIORITY         = "0";

    private static final String DV_RADIO_GROUP = "graphics_priority";
    private static final String ACTION_GRAPHICS_PRIORITY = "graphics_priority";
    private static final String ACTION_VIDEO_PRIORITY    = "video_priority";

    private DolbyVisionSettingManager mDolbyVisionSettingManager;

    // Adjust this value to keep things relatively responsive without janking
    // animations
    private static final int DV_SET_DELAY_MS = 500;
    private final Handler mDelayHandler = new Handler();
    private String mNewDvMode;
    private final Runnable mSetDvRunnable = new Runnable() {
        @Override
        public void run() {
            if (ACTION_VIDEO_PRIORITY.equals(mNewDvMode)) {
                mDolbyVisionSettingManager.setGraphicsPriority(VIDEO_PRIORITY);
            } else if (ACTION_GRAPHICS_PRIORITY.equals(mNewDvMode)) {
                mDolbyVisionSettingManager.setGraphicsPriority(GRAPHICS_PRIORITY);
            }
        }
    };

    public static GraphicsPrioritySettingFragment newInstance() {
        return new GraphicsPrioritySettingFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mDolbyVisionSettingManager = new DolbyVisionSettingManager((Context) getActivity());
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
        screen.setTitle(R.string.dolby_vision_graphics_priority);
        Preference activePref = null;

        final List<Action> dvInfoList = getActions();
        for (final Action dvInfo : dvInfoList) {
            final String dvTag = dvInfo.getKey();
            final RadioPreference radioPreference = new RadioPreference(themedContext);
            radioPreference.setKey(dvTag);
            radioPreference.setPersistent(false);
            radioPreference.setTitle(dvInfo.getTitle());
            radioPreference.setRadioGroup(DV_RADIO_GROUP);
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

            if (dvInfo.isChecked()) {
                mNewDvMode = dvTag;
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
        String curPriority = mDolbyVisionSettingManager.getGraphicsPriority();
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder()
               .key(ACTION_GRAPHICS_PRIORITY)
               .title(getString(R.string.graphics_priority))
               .checked(curPriority.equals("1")).build());
        actions.add(new Action.Builder()
               .key(ACTION_VIDEO_PRIORITY)
               .title(getString(R.string.video_priority))
               .checked(curPriority.equals("0")).build());
        return actions;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            if (radioPreference.isChecked()) {
                mNewDvMode = radioPreference.getKey().toString();
                mDelayHandler.removeCallbacks(mSetDvRunnable);
                mDelayHandler.postDelayed(mSetDvRunnable, DV_SET_DELAY_MS);
            } else {
                radioPreference.setChecked(true);
            }
        }
        return super.onPreferenceTreeClick(preference);
    }
}
