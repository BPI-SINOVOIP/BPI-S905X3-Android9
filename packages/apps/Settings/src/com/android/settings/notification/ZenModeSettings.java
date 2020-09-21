/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.settings.notification;

import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_ALARMS;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_CALLS;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_EVENTS;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_MEDIA;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_MESSAGES;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_REMINDERS;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_REPEAT_CALLERS;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM;

import android.app.AutomaticZenRule;
import android.app.FragmentManager;
import android.app.NotificationManager;
import android.app.NotificationManager.Policy;
import android.content.Context;
import android.icu.text.ListFormatter;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.service.notification.ZenModeConfig;
import android.support.annotation.VisibleForTesting;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.lifecycle.Lifecycle;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.function.Predicate;

public class ZenModeSettings extends ZenModeSettingsBase {
    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.zen_mode_settings;
    }

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.NOTIFICATION_ZEN_MODE;
    }

    @Override
    protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
        return buildPreferenceControllers(context, getLifecycle(), getFragmentManager());
    }

    @Override
    public int getHelpResource() {
        return R.string.help_uri_interruptions;
    }

    private static List<AbstractPreferenceController> buildPreferenceControllers(Context context,
            Lifecycle lifecycle, FragmentManager fragmentManager) {
        List<AbstractPreferenceController> controllers = new ArrayList<>();
        controllers.add(new ZenModeBehaviorMsgEventReminderPreferenceController(context, lifecycle));
        controllers.add(new ZenModeBehaviorSoundPreferenceController(context, lifecycle));
        controllers.add(new ZenModeBehaviorCallsPreferenceController(context, lifecycle));
        controllers.add(new ZenModeBlockedEffectsPreferenceController(context, lifecycle));
        controllers.add(new ZenModeDurationPreferenceController(context, lifecycle,
                fragmentManager));
        controllers.add(new ZenModeAutomationPreferenceController(context));
        controllers.add(new ZenModeButtonPreferenceController(context, lifecycle, fragmentManager));
        controllers.add(new ZenModeSettingsFooterPreferenceController(context, lifecycle));
        return controllers;
    }

    public static class SummaryBuilder {

        private Context mContext;

        public SummaryBuilder(Context context) {
            mContext = context;
        }

        // these should match NotificationManager.Policy#ALL_PRIORITY_CATEGORIES
        private static final int[] ALL_PRIORITY_CATEGORIES = {
                PRIORITY_CATEGORY_ALARMS,
                PRIORITY_CATEGORY_MEDIA,
                PRIORITY_CATEGORY_SYSTEM,
                PRIORITY_CATEGORY_MESSAGES,
                PRIORITY_CATEGORY_EVENTS,
                PRIORITY_CATEGORY_REMINDERS,
                PRIORITY_CATEGORY_CALLS,
                PRIORITY_CATEGORY_REPEAT_CALLERS,
        };

        String getSoundSettingSummary(Policy policy) {
            List<String> enabledCategories = getEnabledCategories(policy,
                    category -> PRIORITY_CATEGORY_ALARMS == category
                            || PRIORITY_CATEGORY_MEDIA == category
                            || PRIORITY_CATEGORY_SYSTEM == category);
            int numCategories = enabledCategories.size();
            if (numCategories == 0) {
                return mContext.getString(R.string.zen_sound_all_muted);
            } else if (numCategories == 1) {
                return mContext.getString(R.string.zen_sound_one_allowed,
                        enabledCategories.get(0).toLowerCase());
            } else if (numCategories == 2) {
                return mContext.getString(R.string.zen_sound_two_allowed,
                        enabledCategories.get(0).toLowerCase(),
                        enabledCategories.get(1).toLowerCase());
            } else if (numCategories == 3) {
                return mContext.getString(R.string.zen_sound_three_allowed,
                        enabledCategories.get(0).toLowerCase(),
                        enabledCategories.get(1).toLowerCase(),
                        enabledCategories.get(2).toLowerCase());
            } else {
                return mContext.getString(R.string.zen_sound_none_muted);
            }
        }

        String getCallsSettingSummary(Policy policy) {
            List<String> enabledCategories = getEnabledCategories(policy,
                    category -> PRIORITY_CATEGORY_CALLS == category
                            || PRIORITY_CATEGORY_REPEAT_CALLERS == category);
            int numCategories = enabledCategories.size();
            if (numCategories == 0) {
                return mContext.getString(R.string.zen_mode_no_exceptions);
            } else if (numCategories == 1) {
                return mContext.getString(R.string.zen_mode_calls_summary_one,
                        enabledCategories.get(0).toLowerCase());
            } else {
                return mContext.getString(R.string.zen_mode_calls_summary_two,
                        enabledCategories.get(0).toLowerCase(),
                        enabledCategories.get(1).toLowerCase());
            }
        }

        String getMsgEventReminderSettingSummary(Policy policy) {
            List<String> enabledCategories = getEnabledCategories(policy,
                    category -> PRIORITY_CATEGORY_EVENTS == category
                            || PRIORITY_CATEGORY_REMINDERS == category
                            || PRIORITY_CATEGORY_MESSAGES == category);
            int numCategories = enabledCategories.size();
            if (numCategories == 0) {
                return mContext.getString(R.string.zen_mode_no_exceptions);
            } else if (numCategories == 1) {
                return enabledCategories.get(0);
            } else if (numCategories == 2) {
                return mContext.getString(R.string.join_two_items, enabledCategories.get(0),
                        enabledCategories.get(1).toLowerCase());
            } else if (numCategories == 3){
                final List<String> summaries = new ArrayList<>();
                summaries.add(enabledCategories.get(0));
                summaries.add(enabledCategories.get(1).toLowerCase());
                summaries.add(enabledCategories.get(2).toLowerCase());

                return ListFormatter.getInstance().format(summaries);
            } else {
                final List<String> summaries = new ArrayList<>();
                summaries.add(enabledCategories.get(0));
                summaries.add(enabledCategories.get(1).toLowerCase());
                summaries.add(enabledCategories.get(2).toLowerCase());
                summaries.add(mContext.getString(R.string.zen_mode_other_options));

                return ListFormatter.getInstance().format(summaries);
            }
        }

        String getSoundSummary() {
            int zenMode = NotificationManager.from(mContext).getZenMode();

            if (zenMode != Settings.Global.ZEN_MODE_OFF) {
                ZenModeConfig config = NotificationManager.from(mContext).getZenModeConfig();
                String description = ZenModeConfig.getDescription(mContext, true, config, false);

                if (description == null) {
                    return mContext.getString(R.string.zen_mode_sound_summary_on);
                } else {
                    return mContext.getString(R.string.zen_mode_sound_summary_on_with_info,
                            description);
                }
            } else {
                final int count = getEnabledAutomaticRulesCount();
                if (count > 0) {
                    return mContext.getString(R.string.zen_mode_sound_summary_off_with_info,
                            mContext.getResources().getQuantityString(
                                    R.plurals.zen_mode_sound_summary_summary_off_info,
                                    count, count));
                }

                return mContext.getString(R.string.zen_mode_sound_summary_off);
            }
        }

        String getBlockedEffectsSummary(Policy policy) {
            if (policy.suppressedVisualEffects == 0) {
                return mContext.getResources().getString(
                        R.string.zen_mode_restrict_notifications_summary_muted);
            } else if (Policy.areAllVisualEffectsSuppressed(policy.suppressedVisualEffects)) {
                return mContext.getResources().getString(
                        R.string.zen_mode_restrict_notifications_summary_hidden);
            } else {
                return mContext.getResources().getString(
                        R.string.zen_mode_restrict_notifications_summary_custom);
            }
        }

        String getAutomaticRulesSummary() {
            final int count = getEnabledAutomaticRulesCount();
            return count == 0 ? mContext.getString(R.string.zen_mode_settings_summary_off)
                : mContext.getResources().getQuantityString(
                    R.plurals.zen_mode_settings_summary_on, count, count);
        }

        @VisibleForTesting
        int getEnabledAutomaticRulesCount() {
            int count = 0;
            final Map<String, AutomaticZenRule> ruleMap =
                NotificationManager.from(mContext).getAutomaticZenRules();
            if (ruleMap != null) {
                for (Entry<String, AutomaticZenRule> ruleEntry : ruleMap.entrySet()) {
                    final AutomaticZenRule rule = ruleEntry.getValue();
                    if (rule != null && rule.isEnabled()) {
                        count++;
                    }
                }
            }
            return count;
        }

        private List<String> getEnabledCategories(Policy policy,
                Predicate<Integer> filteredCategories) {
            List<String> enabledCategories = new ArrayList<>();
            for (int category : ALL_PRIORITY_CATEGORIES) {
                if (filteredCategories.test(category) && isCategoryEnabled(policy, category)) {
                    if (category == PRIORITY_CATEGORY_ALARMS) {
                        enabledCategories.add(mContext.getString(R.string.zen_mode_alarms));
                    } else if (category == PRIORITY_CATEGORY_MEDIA) {
                        enabledCategories.add(mContext.getString(
                                R.string.zen_mode_media));
                    } else if (category == PRIORITY_CATEGORY_SYSTEM) {
                        enabledCategories.add(mContext.getString(
                                R.string.zen_mode_system));
                    } else if (category == Policy.PRIORITY_CATEGORY_MESSAGES) {
                        if (policy.priorityMessageSenders == Policy.PRIORITY_SENDERS_ANY) {
                            enabledCategories.add(mContext.getString(
                                    R.string.zen_mode_all_messages));
                        } else {
                            enabledCategories.add(mContext.getString(
                                    R.string.zen_mode_selected_messages));
                        }
                    } else if (category == Policy.PRIORITY_CATEGORY_EVENTS) {
                        enabledCategories.add(mContext.getString(R.string.zen_mode_events));
                    } else if (category == Policy.PRIORITY_CATEGORY_REMINDERS) {
                        enabledCategories.add(mContext.getString(R.string.zen_mode_reminders));
                    } else if (category == Policy.PRIORITY_CATEGORY_CALLS) {
                        if (policy.priorityCallSenders == Policy.PRIORITY_SENDERS_ANY) {
                            enabledCategories.add(mContext.getString(
                                    R.string.zen_mode_all_callers));
                        } else if (policy.priorityCallSenders == Policy.PRIORITY_SENDERS_CONTACTS){
                            enabledCategories.add(mContext.getString(
                                    R.string.zen_mode_contacts_callers));
                        } else {
                            enabledCategories.add(mContext.getString(
                                    R.string.zen_mode_starred_callers));
                        }
                    } else if (category == Policy.PRIORITY_CATEGORY_REPEAT_CALLERS) {
                        if (!enabledCategories.contains(mContext.getString(
                                R.string.zen_mode_all_callers))) {
                            enabledCategories.add(mContext.getString(
                                    R.string.zen_mode_repeat_callers));
                        }
                    }
                }
            }
            return enabledCategories;
        }

        private boolean isCategoryEnabled(Policy policy, int categoryType) {
            return (policy.priorityCategories & categoryType) != 0;
        }
    }

    /**
     * For Search.
     */
    public static final Indexable.SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {

                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
                        boolean enabled) {
                    final SearchIndexableResource sir = new SearchIndexableResource(context);
                    sir.xmlResId = R.xml.zen_mode_settings;
                    return Arrays.asList(sir);
                }

                @Override
                public List<String> getNonIndexableKeys(Context context) {
                    List<String> keys = super.getNonIndexableKeys(context);
                    keys.add(ZenModeDurationPreferenceController.KEY);
                    keys.add(ZenModeButtonPreferenceController.KEY);
                    return keys;
                }

                @Override
                public List<AbstractPreferenceController> createPreferenceControllers(Context
                        context) {
                    return buildPreferenceControllers(context, null, null);
                }
            };
}
