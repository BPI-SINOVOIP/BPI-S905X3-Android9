/* Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.droidlogic.tv.settings.display.dolbyvision;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
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
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class DolbyVisionSettingFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "DolbyVisionSettingFragment";

    public static final String KEY_DOLBY_VISION     = "dolby_vision_set";

    private static final int DV_LL_RGB            = 3;
    private static final int DV_LL_YUV            = 2;
    private static final int DV_ENABLE            = 1;
    private static final int DV_DISABLE           = 0;

    private static final String DV_RADIO_GROUP = "dv";
    private static final String DOLBY_VISION_DEFAULT = "dolby_vision_default";
    private static final String DOLBY_VISION_LL_YUV  = "dolby_vision_ll_yuv";
    private static final String DOLBY_VISION_LL_RGB  = "dolby_vision_ll_rgb";
    private static final String DOLBY_VISION_DISABLE = "dolby_vision_disable";

    private DolbyVisionSettingManager mDolbyVisionSettingManager;
    private OutputModeManager mOutputModeManager;

    // Adjust this value to keep things relatively responsive without janking
    // animations
    private static final int DV_SET_DELAY_MS = 500;
    private final Handler mDelayHandler = new Handler();
    private String mNewDvMode;
    Intent serviceIntent;
    private final Runnable mSetDvRunnable = new Runnable() {
        @Override
        public void run() {
            if (!mDolbyVisionSettingManager.isTvSupportDolbyVision().equals("")) {
                mOutputModeManager.setBestDolbyVision(false);
            }
            if (DOLBY_VISION_DEFAULT.equals(mNewDvMode)) {
                mDolbyVisionSettingManager.setDolbyVisionEnable(DV_ENABLE);
                serviceIntent = new Intent(getPreferenceManager().getContext(), DolbyVisionService.class);
                getPreferenceManager().getContext().startService(serviceIntent);
            } else if (DOLBY_VISION_LL_YUV.equals(mNewDvMode)) {
                mDolbyVisionSettingManager.setDolbyVisionEnable(DV_LL_YUV);
                serviceIntent = new Intent(getPreferenceManager().getContext(), DolbyVisionService.class);
                getPreferenceManager().getContext().startService(serviceIntent);
            } else if (DOLBY_VISION_LL_RGB.equals(mNewDvMode)) {
                mDolbyVisionSettingManager.setDolbyVisionEnable(DV_LL_RGB);
                serviceIntent = new Intent(getPreferenceManager().getContext(), DolbyVisionService.class);
                getPreferenceManager().getContext().startService(serviceIntent);
            } else if (DOLBY_VISION_DISABLE.equals(mNewDvMode)) {
                mDolbyVisionSettingManager.setDolbyVisionEnable(DV_DISABLE);
                if (serviceIntent != null) {
                    getPreferenceManager().getContext().stopService(serviceIntent);
                }
            }
        }
    };

    public static DolbyVisionSettingFragment newInstance() {
        return new DolbyVisionSettingFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mDolbyVisionSettingManager = new DolbyVisionSettingManager((Context) getActivity());
        mOutputModeManager = new OutputModeManager((Context) getActivity());
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
        screen.setTitle(R.string.dolby_vision_set);
        String currentDvMode = null;
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
        boolean enable = mDolbyVisionSettingManager.isDolbyVisionEnable();
        int type = mDolbyVisionSettingManager.getDolbyVisionType();
        String mode = mDolbyVisionSettingManager.isTvSupportDolbyVision();
        String curMode = mOutputModeManager.getCurrentOutputMode();
        ArrayList<Action> actions = new ArrayList<Action>();
        if (mode.equals("")) {
            actions.add(new Action.Builder()
                .key(DOLBY_VISION_DEFAULT)
                .title(getString(R.string.dolby_vision_default_enable))
                .checked((enable == true) && (type == DV_ENABLE))
                .build());
        }
        if (!mode.equals("") && mode.contains("DV_RGB_444_8BIT")) {
            actions.add(new Action.Builder()
                .key(DOLBY_VISION_DEFAULT)
                .title(getString(R.string.dolby_vision_sink_led))
                .checked((enable == true) && (type == DV_ENABLE))
                .build());
        }
        if (!mode.equals("")) {
            if (mode.contains("LL_YCbCr_422_12BIT")) {
                actions.add(new Action.Builder()
                    .key(DOLBY_VISION_LL_YUV)
                    .title(getString(R.string.dolby_vision_low_latency_yuv))
                    .checked((enable == true) && (type == DV_LL_YUV))
                    .build());
            }
            if ((mode.contains("LL_RGB_444_12BIT") || mode.contains("LL_RGB_444_10BIT"))
                    && !curMode.contains("2160") && !curMode.contains("smpte")) {
                actions.add(new Action.Builder()
                    .key(DOLBY_VISION_LL_RGB)
                    .title(getString(R.string.dolby_vision_low_latency_rgb))
                    .checked((enable == true) && (type == DV_LL_RGB))
                    .build());
            }
        }
        actions.add(new Action.Builder()
            .key(DOLBY_VISION_DISABLE)
            .title(getString(R.string.dolby_vision_off))
            .checked(enable == false)
            .build());
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
