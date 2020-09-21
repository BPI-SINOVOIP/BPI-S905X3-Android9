/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.compatibility.common.util;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.AssumptionViolatedException;

/**
 * Helper and constants accessible to host and device components that enable Business Logic
 * configuration
 */
public class BusinessLogic {

    // Device location to which business logic data is pushed
    public static final String DEVICE_FILE = "/sdcard/bl";

    /* A map from testcase name to the business logic rules for the test case */
    protected Map<String, List<BusinessLogicRulesList>> mRules;
    /* Feature flag determining if device specific tests are executed. */
    public boolean mConditionalTestsEnabled;
    private AuthenticationStatusEnum mAuthenticationStatus = AuthenticationStatusEnum.UNKNOWN;

    // A Date denoting the time of request from the business logic service
    protected Date mTimestamp;

    /**
     * Determines whether business logic exists for a given test name
     * @param testName the name of the test case, prefixed by fully qualified class name, then '#'.
     * For example, "com.android.foo.FooTest#testFoo"
     * @return whether business logic exists for this test for this suite
     */
    public boolean hasLogicFor(String testName) {
        List<BusinessLogicRulesList> rulesLists = mRules.get(testName);
        return rulesLists != null && !rulesLists.isEmpty();
    }

    /**
     * Return whether multiple rule lists exist in the BusinessLogic for this test name.
     */
    private boolean hasLogicsFor(String testName) {
        List<BusinessLogicRulesList> rulesLists = mRules.get(testName);
        return rulesLists != null && rulesLists.size() > 1;
    }

    /**
     * Apply business logic for the given test.
     * @param testName the name of the test case, prefixed by fully qualified class name, then '#'.
     * For example, "com.android.foo.FooTest#testFoo"
     * @param executor a {@link BusinessLogicExecutor}
     */
    public void applyLogicFor(String testName, BusinessLogicExecutor executor) {
        if (!hasLogicFor(testName)) {
            return;
        }
        if (hasLogicsFor(testName)) {
            applyLogicsFor(testName, executor); // handle this special case separately
            return;
        }
        // expecting exactly one rules list at this point
        BusinessLogicRulesList rulesList = mRules.get(testName).get(0);
        rulesList.invokeRules(executor);
    }

    /**
     * Handle special case in which multiple rule lists exist for the test name provided.
     * Execute each rule list in a sandbox and store an exception for each rule list that
     * triggers failure or skipping for the test.
     * If all rule lists trigger skipping, rethrow AssumptionViolatedException to report a 'skip'
     * for the test as a whole.
     * If one or more rule lists trigger failure, rethrow RuntimeException with a list containing
     * each failure.
     */
    private void applyLogicsFor(String testName, BusinessLogicExecutor executor) {
        Map<String, RuntimeException> failedMap = new HashMap<>();
        Map<String, RuntimeException> skippedMap = new HashMap<>();
        List<BusinessLogicRulesList> rulesLists = mRules.get(testName);
        for (int index = 0; index < rulesLists.size(); index++) {
            BusinessLogicRulesList rulesList = rulesLists.get(index);
            String description = cleanDescription(rulesList.getDescription(), index);
            try {
                rulesList.invokeRules(executor);
            } catch (RuntimeException re) {
                if (AssumptionViolatedException.class.isInstance(re)) {
                    skippedMap.put(description, re);
                    executor.logInfo("Test %s (%s) skipped for reason: %s", testName, description,
                            re.getMessage());
                } else {
                    failedMap.put(description, re);
                }
            }
        }
        if (skippedMap.size() == rulesLists.size()) {
            throwAggregatedException(skippedMap, false);
        } else if (failedMap.size() > 0) {
            throwAggregatedException(failedMap, true);
        } // else this test should be reported as a pure pass
    }

    /**
     * Helper to aggregate the messages of many {@link RuntimeException}s, and optionally their
     * stack traces, before throwing an exception.
     * @param exceptions a map from description strings to exceptions. The descriptive keySet is
     * used to differentiate which BusinessLogicRulesList caused which exception
     * @param failed whether to trigger failure. When false, throws assumption failure instead, and
     * excludes stack traces from the exception message.
     */
    private static void throwAggregatedException(Map<String, RuntimeException> exceptions,
            boolean failed) {
        Set<String> keySet = exceptions.keySet();
        String[] descriptions = keySet.toArray(new String[keySet.size()]);
        StringBuilder msg = new StringBuilder("");
        msg.append(String.format("Test %s for cases: ", (failed) ? "failed" : "skipped"));
        msg.append(String.join(", ", descriptions));
        msg.append("\nReasons include:");
        for (String description : descriptions) {
            RuntimeException re = exceptions.get(description);
            msg.append(String.format("\nMessage [%s]: %s", description, re.getMessage()));
            if (failed) {
                StringWriter sw = new StringWriter();
                re.printStackTrace(new PrintWriter(sw));
                msg.append(String.format("\nStack Trace: %s", sw.toString()));
            }
        }
        if (failed) {
            throw new RuntimeException(msg.toString());
        } else {
            throw new AssumptionViolatedException(msg.toString());
        }
    }

    /**
     * Helper method to generate a meaningful description in case the provided description is null
     * or empty. In this case, returns a string representation of the index provided.
     */
    private String cleanDescription(String description, int index) {
        return (description == null || description.length() == 0)
                ? Integer.toString(index)
                : description;
    }

    public void setAuthenticationStatus(String authenticationStatus) {
        try {
            mAuthenticationStatus = Enum.valueOf(AuthenticationStatusEnum.class,
                    authenticationStatus);
        } catch (IllegalArgumentException e) {
            // Invalid value, set to unknown
            mAuthenticationStatus = AuthenticationStatusEnum.UNKNOWN;
        }
    }

    public boolean isAuthorized() {
        return AuthenticationStatusEnum.AUTHORIZED.equals(mAuthenticationStatus);
    }

    public Date getTimestamp() {
        return mTimestamp;
    }

    /**
     * Builds a user readable string tha explains the authentication status and the effect on tests
     * which require authentication to execute.
     */
    public String getAuthenticationStatusMessage() {
        switch (mAuthenticationStatus) {
            case AUTHORIZED:
                return "Authorized";
            case NOT_AUTHENTICATED:
                return "authorization failed, please ensure the service account key is "
                        + "properly installed.";
            case NOT_AUTHORIZED:
                return "service account is not authorized to access information for this device. "
                        + "Please verify device properties are set correctly and account "
                        + "permissions are configured to the Business Logic Api.";
            default:
                return "something went wrong, please try again.";
        }
    }

    /**
     * A list of BusinessLogicRules, wrapped with an optional description to differentiate rule
     * lists that apply to the same test.
     */
    protected static class BusinessLogicRulesList {

        /* Stored description and rules */
        protected List<BusinessLogicRule> mRulesList;
        protected String mDescription;

        public BusinessLogicRulesList(List<BusinessLogicRule> rulesList) {
            mRulesList = rulesList;
        }

        public BusinessLogicRulesList(List<BusinessLogicRule> rulesList, String description) {
            mRulesList = rulesList;
            mDescription = description;
        }

        public String getDescription() {
            return mDescription;
        }

        public List<BusinessLogicRule> getRules() {
            return mRulesList;
        }

        public void invokeRules(BusinessLogicExecutor executor) {
            for (BusinessLogicRule rule : mRulesList) {
                // Check conditions
                if (rule.invokeConditions(executor)) {
                    rule.invokeActions(executor);
                }
            }
        }
    }

    /**
     * Nested class representing an Business Logic Rule. Stores a collection of conditions
     * and actions for later invokation.
     */
    protected static class BusinessLogicRule {

        /* Stored conditions and actions */
        protected List<BusinessLogicRuleCondition> mConditions;
        protected List<BusinessLogicRuleAction> mActions;

        public BusinessLogicRule(List<BusinessLogicRuleCondition> conditions,
                List<BusinessLogicRuleAction> actions) {
            mConditions = conditions;
            mActions = actions;
        }

        /**
         * Method that invokes all Business Logic conditions for this rule, and returns true
         * if all conditions evaluate to true.
         */
        public boolean invokeConditions(BusinessLogicExecutor executor) {
            for (BusinessLogicRuleCondition condition : mConditions) {
                if (!condition.invoke(executor)) {
                    return false;
                }
            }
            return true;
        }

        /**
         * Method that invokes all Business Logic actions for this rule
         */
        public void invokeActions(BusinessLogicExecutor executor) {
            for (BusinessLogicRuleAction action : mActions) {
                action.invoke(executor);
            }
        }
    }

    /**
     * Nested class representing an Business Logic Rule Condition. Stores the name of a method
     * to invoke, as well as String args to use during invokation.
     */
    protected static class BusinessLogicRuleCondition {

        /* Stored method name and String args */
        protected String mMethodName;
        protected List<String> mMethodArgs;
        /* Whether or not the boolean result of this condition should be reversed */
        protected boolean mNegated;


        public BusinessLogicRuleCondition(String methodName, List<String> methodArgs,
                boolean negated) {
            mMethodName = methodName;
            mMethodArgs = methodArgs;
            mNegated = negated;
        }

        /**
         * Invoke this Business Logic condition with an executor.
         */
        public boolean invoke(BusinessLogicExecutor executor) {
            // XOR the negated boolean with the return value of the method
            return (mNegated != executor.executeCondition(mMethodName,
                    mMethodArgs.toArray(new String[mMethodArgs.size()])));
        }
    }

    /**
     * Nested class representing an Business Logic Rule Action. Stores the name of a method
     * to invoke, as well as String args to use during invokation.
     */
    protected static class BusinessLogicRuleAction {

        /* Stored method name and String args */
        protected String mMethodName;
        protected List<String> mMethodArgs;

        public BusinessLogicRuleAction(String methodName, List<String> methodArgs) {
            mMethodName = methodName;
            mMethodArgs = methodArgs;
        }

        /**
         * Invoke this Business Logic action with an executor.
         */
        public void invoke(BusinessLogicExecutor executor) {
            executor.executeAction(mMethodName,
                    mMethodArgs.toArray(new String[mMethodArgs.size()]));
        }
    }

    /**
     * Nested enum of the possible authentication statuses.
     */
    protected enum AuthenticationStatusEnum {
        UNKNOWN,
        NOT_AUTHENTICATED,
        NOT_AUTHORIZED,
        AUTHORIZED
    }

}
