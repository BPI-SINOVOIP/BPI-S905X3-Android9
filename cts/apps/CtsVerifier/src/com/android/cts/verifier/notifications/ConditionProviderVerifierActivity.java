/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.cts.verifier.notifications;

import com.android.cts.verifier.R;

import android.app.ActivityManager;
import android.app.AutomaticZenRule;
import android.app.NotificationManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public class ConditionProviderVerifierActivity extends InteractiveVerifierActivity
        implements Runnable {
    protected static final String CP_PACKAGE = "com.android.cts.verifier";
    protected static final String CP_PATH = CP_PACKAGE +
            "/com.android.cts.verifier.notifications.MockConditionProvider";

    @Override
    protected int getTitleResource() {
        return R.string.cp_test;
    }

    @Override
    protected int getInstructionsResource() {
        return R.string.cp_info;
    }

    // Test Setup

    @Override
    protected List<InteractiveTestCase> createTestItems() {
        List<InteractiveTestCase> tests = new ArrayList<>(9);
        ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        if (am.isLowRamDevice()) {
            tests.add(new CannotBeEnabledTest());
            tests.add(new ServiceStoppedTest());
        } else {
            tests.add(new IsEnabledTest());
            tests.add(new ServiceStartedTest());
            tests.add(new CreateAutomaticZenRuleTest());
            tests.add(new UpdateAutomaticZenRuleTest());
            tests.add(new GetAutomaticZenRuleTest());
            tests.add(new GetAutomaticZenRulesTest());
            tests.add(new SubscribeAutomaticZenRuleTest());
            tests.add(new DeleteAutomaticZenRuleTest());
            tests.add(new UnsubscribeAutomaticZenRuleTest());
            tests.add(new RequestUnbindTest());
            tests.add(new RequestBindTest());
            tests.add(new IsDisabledTest());
            tests.add(new ServiceStoppedTest());
        }
        return tests;
    }

    protected class IsEnabledTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createSettingsItem(parent, R.string.cp_enable_service);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            Intent settings = new Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
            if (settings.resolveActivity(mPackageManager) == null) {
                logFail("no settings activity");
                status = FAIL;
            } else {
                if (mNm.isNotificationPolicyAccessGranted()) {
                    status = PASS;
                } else {
                    status = WAIT_FOR_USER;
                }
                next();
            }
        }

        protected void tearDown() {
            // wait for the service to start
            delay();
        }

        @Override
        protected Intent getIntent() {
            return new Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
        }
    }

    protected class CannotBeEnabledTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createNlsSettingsItem(parent, R.string.cp_cannot_enable_service);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            mNm.cancelAll();
            Intent settings = new Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
            if (settings.resolveActivity(mPackageManager) == null) {
                logFail("no settings activity");
                status = FAIL;
            } else {
                if (mNm.isNotificationPolicyAccessGranted()) {
                    status = FAIL;
                } else {
                    status = PASS;
                }
                next();
            }
        }

        protected void tearDown() {
            // wait for the service to start
            delay();
        }

        @Override
        protected Intent getIntent() {
            return new Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
        }
    }

    protected class ServiceStartedTest extends InteractiveTestCase {
        int mRetries = 5;
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_service_started);
        }

        @Override
        protected void test() {
            if (MockConditionProvider.getInstance().isConnected()
                    && MockConditionProvider.getInstance().isBound()) {
                status = PASS;
                next();
            } else {
                if (--mRetries > 0) {
                    status = RETEST;
                    next();
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }
    }

    private class RequestUnbindTest extends InteractiveTestCase {
        int mRetries = 5;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_snooze);

        }

        @Override
        protected void setUp() {
            status = READY;
        }

        @Override
        protected void test() {
            if (status == READY) {
                MockConditionProvider.getInstance().requestUnbind();
                status = RETEST;
            } else {
                if (MockConditionProvider.getInstance() == null ||
                        !MockConditionProvider.getInstance().isConnected()) {
                    status = PASS;
                } else {
                    if (--mRetries > 0) {
                        status = RETEST;
                    } else {
                        logFail();
                        status = FAIL;
                    }
                }
                next();
            }
        }
    }

    private class RequestBindTest extends InteractiveTestCase {
        int mRetries = 5;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_unsnooze);

        }

        @Override
        protected void test() {
            if (status == READY) {
                MockConditionProvider.requestRebind(MockConditionProvider.COMPONENT_NAME);
                status = RETEST;
            } else {
                if (MockConditionProvider.getInstance().isConnected()
                        && MockConditionProvider.getInstance().isBound()) {
                    status = PASS;
                    next();
                } else {
                    if (--mRetries > 0) {
                        status = RETEST;
                        next();
                    } else {
                        logFail();
                        status = FAIL;
                    }
                }
            }
        }
    }


    private class CreateAutomaticZenRuleTest extends InteractiveTestCase {
        private String id = null;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_create_rule);
        }

        @Override
        protected void test() {
            AutomaticZenRule ruleToCreate =
                    createRule("Rule", "value", NotificationManager.INTERRUPTION_FILTER_ALARMS);
            id = mNm.addAutomaticZenRule(ruleToCreate);

            if (!TextUtils.isEmpty(id)) {
                status = PASS;
            } else {
                logFail();
                status = FAIL;
            }
            next();
        }

        @Override
        protected void tearDown() {
            if (id != null) {
                mNm.removeAutomaticZenRule(id);
            }
            MockConditionProvider.getInstance().resetData();
        }
    }

    private class UpdateAutomaticZenRuleTest extends InteractiveTestCase {
        private String id = null;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_update_rule);
        }

        @Override
        protected void setUp() {
            id = mNm.addAutomaticZenRule(createRule("BeforeUpdate", "beforeValue",
                    NotificationManager.INTERRUPTION_FILTER_ALARMS));
            status = READY;
            delay();
        }

        @Override
        protected void test() {
            AutomaticZenRule updated = mNm.getAutomaticZenRule(id);
            updated.setName("AfterUpdate");
            updated.setConditionId(MockConditionProvider.toConditionId("afterValue"));
            updated.setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_NONE);

            try {
                boolean success = mNm.updateAutomaticZenRule(id, updated);
                if (success && updated.equals(mNm.getAutomaticZenRule(id))) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            } catch (Exception e) {
                logFail("update failed", e);
                status = FAIL;
            }
            next();
        }

        @Override
        protected void tearDown() {
            if (id != null) {
                mNm.removeAutomaticZenRule(id);
            }
            MockConditionProvider.getInstance().resetData();
        }
    }

    private class SubscribeAutomaticZenRuleTest extends InteractiveTestCase {
        private String id = null;
        private AutomaticZenRule ruleToCreate;
        private int mRetries = 3;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_subscribe_rule);
        }

        @Override
        protected void setUp() {
            ruleToCreate = createRule("RuleSubscribe", "Subscribevalue",
                    NotificationManager.INTERRUPTION_FILTER_ALARMS);
            id = mNm.addAutomaticZenRule(ruleToCreate);
            status = READY;
            delay();
        }

        @Override
        protected void test() {
            boolean foundMatch = false;
            List<Uri> subscriptions = MockConditionProvider.getInstance().getSubscriptions();
            for (Uri actual : subscriptions) {
                if (ruleToCreate.getConditionId().equals(actual)) {
                    status = PASS;
                    foundMatch = true;
                    break;
                }
            }
            if (foundMatch) {
                status = PASS;
                next();
            } else if (--mRetries > 0) {
                setFailed();
            } else {
                status = RETEST;
                next();
            }
        }

        @Override
        protected void tearDown() {
            if (id != null) {
                mNm.removeAutomaticZenRule(id);
            }
            MockConditionProvider.getInstance().resetData();
        }
    }

    private class GetAutomaticZenRuleTest extends InteractiveTestCase {
        private String id = null;
        private AutomaticZenRule ruleToCreate;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_get_rule);
        }

        @Override
        protected void setUp() {
            ruleToCreate = createRule("RuleGet", "valueGet",
                    NotificationManager.INTERRUPTION_FILTER_ALARMS);
            id = mNm.addAutomaticZenRule(ruleToCreate);
            status = READY;
            delay();
        }

        @Override
        protected void test() {
            AutomaticZenRule queriedRule = mNm.getAutomaticZenRule(id);
            if (queriedRule != null
                    && ruleToCreate.getName().equals(queriedRule.getName())
                    && ruleToCreate.getOwner().equals(queriedRule.getOwner())
                    && ruleToCreate.getConditionId().equals(queriedRule.getConditionId())
                    && ruleToCreate.isEnabled() == queriedRule.isEnabled()) {
                status = PASS;
            } else {
                logFail();
                status = FAIL;
            }
            next();
        }

        @Override
        protected void tearDown() {
            if (id != null) {
                mNm.removeAutomaticZenRule(id);
            }
            MockConditionProvider.getInstance().resetData();
        }
    }

    private class GetAutomaticZenRulesTest extends InteractiveTestCase {
        private List<String> ids = new ArrayList<>();
        private AutomaticZenRule rule1;
        private AutomaticZenRule rule2;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_get_rules);
        }

        @Override
        protected void setUp() {
            rule1 = createRule("Rule1", "value1", NotificationManager.INTERRUPTION_FILTER_ALARMS);
            rule2 = createRule("Rule2", "value2", NotificationManager.INTERRUPTION_FILTER_NONE);
            ids.add(mNm.addAutomaticZenRule(rule1));
            ids.add(mNm.addAutomaticZenRule(rule2));
            status = READY;
            delay();
        }

        @Override
        protected void test() {
            Map<String, AutomaticZenRule> rules = mNm.getAutomaticZenRules();

            if (rules == null || rules.size() != 2) {
                logFail();
                status = FAIL;
                next();
                return;
            }

            for (AutomaticZenRule createdRule : rules.values()) {
                if (!compareRules(createdRule, rule1) && !compareRules(createdRule, rule2)) {
                    logFail();
                    status = FAIL;
                    break;
                }
            }
            status = PASS;
            next();
        }

        @Override
        protected void tearDown() {
            for (String id : ids) {
                mNm.removeAutomaticZenRule(id);
            }
            MockConditionProvider.getInstance().resetData();
        }
    }

    private class DeleteAutomaticZenRuleTest extends InteractiveTestCase {
        private String id = null;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_delete_rule);
        }

        @Override
        protected void test() {
            AutomaticZenRule ruleToCreate = createRule("RuleDelete", "Deletevalue",
                    NotificationManager.INTERRUPTION_FILTER_ALARMS);
            id = mNm.addAutomaticZenRule(ruleToCreate);

            if (id != null) {
                if (mNm.removeAutomaticZenRule(id)) {
                    if (mNm.getAutomaticZenRule(id) == null) {
                        status = PASS;
                    } else {
                        logFail();
                        status = FAIL;
                    }
                } else {
                    logFail();
                    status = FAIL;
                }
            } else {
                logFail("Couldn't test rule deletion; creation failed.");
                status = FAIL;
            }
            next();
        }

        @Override
        protected void tearDown() {
            MockConditionProvider.getInstance().resetData();
            delay();
        }
    }

    private class UnsubscribeAutomaticZenRuleTest extends InteractiveTestCase {
        private String id = null;
        private AutomaticZenRule ruleToCreate;
        private int mSubscribeRetries = 3;
        private int mUnsubscribeRetries = 3;
        private boolean mSubscribing = true;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_unsubscribe_rule);
        }

        @Override
        protected void setUp() {
            ruleToCreate = createRule("RuleUnsubscribe", "valueUnsubscribe",
                    NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            id = mNm.addAutomaticZenRule(ruleToCreate);
            status = READY;
            delay();
        }

        @Override
        protected void test() {
            if (mSubscribing) {
                // trying to subscribe
                boolean foundMatch = false;
                List<Uri> subscriptions = MockConditionProvider.getInstance().getSubscriptions();
                for (Uri actual : subscriptions) {
                    if (ruleToCreate.getConditionId().equals(actual)) {
                        status = PASS;
                        foundMatch = true;
                        break;
                    }
                }
                if (foundMatch) {
                    // Now that it's subscribed, remove the rule and verify that it
                    // unsubscribes.
                    Log.d(MockConditionProvider.TAG, "Found subscription, removing");
                    mNm.removeAutomaticZenRule(id);

                    mSubscribing = false;
                    status = RETEST;
                    next();
                } else if (--mSubscribeRetries > 0) {
                    setFailed();
                } else {
                    status = RETEST;
                    next();
                }
            } else {
                // trying to unsubscribe
                List<Uri> continuingSubscriptions
                        = MockConditionProvider.getInstance().getSubscriptions();
                boolean stillFoundMatch = false;
                for (Uri actual : continuingSubscriptions) {
                    if (ruleToCreate.getConditionId().equals(actual)) {
                        stillFoundMatch = true;
                        break;
                    }
                }
                if (!stillFoundMatch) {
                    status = PASS;
                    next();
                } else if (stillFoundMatch && --mUnsubscribeRetries > 0) {
                    Log.d(MockConditionProvider.TAG, "Still found subscription, retrying");
                    status = RETEST;
                    next();
                } else {
                    setFailed();
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.removeAutomaticZenRule(id);
            MockConditionProvider.getInstance().resetData();
        }
    }

    private class IsDisabledTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createSettingsItem(parent, R.string.cp_disable_service);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            if (!mNm.isNotificationPolicyAccessGranted()) {
                status = PASS;
            } else {
                status = WAIT_FOR_USER;
            }
            next();
        }

        @Override
        protected Intent getIntent() {
            return new Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
        }
    }

    private class ServiceStoppedTest extends InteractiveTestCase {
        int mRetries = 5;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.cp_service_stopped);
        }

        @Override
        protected void test() {
            if (MockConditionProvider.getInstance() == null ||
                    !MockConditionProvider.getInstance().isConnected()) {
                status = PASS;
            } else {
                if (--mRetries > 0) {
                    status = RETEST;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
            next();
        }
    }

    private AutomaticZenRule createRule(String name, String queryValue, int status) {
        return new AutomaticZenRule(name,
                ComponentName.unflattenFromString(CP_PATH),
                MockConditionProvider.toConditionId(queryValue),
                status,
                true);
    }

    private boolean compareRules(AutomaticZenRule rule1, AutomaticZenRule rule2) {
        return rule1.isEnabled() == rule2.isEnabled()
                && Objects.equals(rule1.getName(), rule2.getName())
                && rule1.getInterruptionFilter() == rule2.getInterruptionFilter()
                && Objects.equals(rule1.getConditionId(), rule2.getConditionId())
                && Objects.equals(rule1.getOwner(), rule2.getOwner());
    }

    protected View createSettingsItem(ViewGroup parent, int messageId) {
        return createUserItem(parent, R.string.cp_start_settings, messageId);
    }
}
