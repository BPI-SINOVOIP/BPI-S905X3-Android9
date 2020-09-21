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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.android.compatibility.common.util.BusinessLogic.BusinessLogicRule;
import com.android.compatibility.common.util.BusinessLogic.BusinessLogicRuleAction;
import com.android.compatibility.common.util.BusinessLogic.BusinessLogicRuleCondition;
import com.android.compatibility.common.util.BusinessLogic.BusinessLogicRulesList;

import java.io.File;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.TimeZone;

/**
 * Factory for creating a {@link BusinessLogic}
 */
public class BusinessLogicFactory {

    // Name of list object storing test-rules pairs
    private static final String BUSINESS_LOGIC_RULES_LISTS = "businessLogicRulesLists";
    // Name of test name string
    private static final String TEST_NAME = "testName";
    // Name of rules object (one 'rules' object to a single test)
    private static final String BUSINESS_LOGIC_RULES = "businessLogicRules";
    // Name of rule conditions array
    private static final String RULE_CONDITIONS = "ruleConditions";
    // Name of rule actions array
    private static final String RULE_ACTIONS = "ruleActions";
    // Description of a rule list object
    private static final String RULES_LIST_DESCRIPTION = "description";
    // Name of method name string
    private static final String METHOD_NAME = "methodName";
    // Name of method args array of strings
    private static final String METHOD_ARGS = "methodArgs";
    // Name of the field in the response object that stores that the auth status of the request.
    private static final String AUTHENTICATION_STATUS = "authenticationStatus";
    public static final String CONDITIONAL_TESTS_ENABLED = "conditionalTestsEnabled";
    // Name of the timestamp field
    private static final String TIMESTAMP = "timestamp";
    // Date and time pattern for raw timestamp string
    private static final String TIMESTAMP_PATTERN = "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";

    /**
     * Create a BusinessLogic instance from a file of business logic data, formatted in JSON.
     * This format is identical to that which is received from the Android Partner business logic
     * service.
     */
    public static BusinessLogic createFromFile(File f) {
        // Populate the map from testname to business rules for this new BusinessLogic instance
        Map<String, List<BusinessLogicRulesList>> rulesMap = new HashMap<>();
        BusinessLogic bl = new BusinessLogic();
        try {
            String businessLogicString = readFile(f);
            JSONObject root = new JSONObject(businessLogicString);
            JSONArray jsonRulesLists = null;
            if (root.has(AUTHENTICATION_STATUS)){
                String authStatus = root.getString(AUTHENTICATION_STATUS);
                bl.setAuthenticationStatus(authStatus);
            }
            if (root.has(CONDITIONAL_TESTS_ENABLED)){
                boolean enabled = root.getBoolean(CONDITIONAL_TESTS_ENABLED);
                bl.mConditionalTestsEnabled = enabled;
            }
            if (root.has(TIMESTAMP)) {
                bl.mTimestamp = parseTimestamp(root.getString(TIMESTAMP));
            }
            try {
                jsonRulesLists = root.getJSONArray(BUSINESS_LOGIC_RULES_LISTS);
            } catch (JSONException e) {
                bl.mRules = rulesMap;
                return bl; // no rules defined for this suite, leave internal map empty
            }
            for (int i = 0; i < jsonRulesLists.length(); i++) {
                JSONObject jsonRulesList = jsonRulesLists.getJSONObject(i);
                String testName = jsonRulesList.getString(TEST_NAME);
                List<BusinessLogicRulesList> testRulesLists = rulesMap.get(testName);
                if (testRulesLists == null) {
                    testRulesLists = new ArrayList<>();
                }
                testRulesLists.add(extractRulesList(jsonRulesList));
                rulesMap.put(testName, testRulesLists);
            }
        } catch (IOException | JSONException e) {
            throw new RuntimeException("Business Logic failed", e);
        }
        // Return business logic
        bl.mRules = rulesMap;
        return bl;
    }

    /* Extract a BusinessLogicRulesList from the representative JSON object */
    private static BusinessLogicRulesList extractRulesList(JSONObject rulesListJSONObject)
            throws JSONException {
        // First, parse the description for this rule list object, if one exists
        String description = null;
        try {
            description = rulesListJSONObject.getString(RULES_LIST_DESCRIPTION);
        } catch (JSONException e) { /* no description set, leave null */}

        // Next, get the list of rules
        List<BusinessLogicRule> rules = new ArrayList<>();
        JSONArray rulesJSONArray = null;
        try {
            rulesJSONArray = rulesListJSONObject.getJSONArray(BUSINESS_LOGIC_RULES);
        } catch (JSONException e) {
            // no rules defined for this test case, return new, rule-less BusinessLogicRulesList
            return new BusinessLogicRulesList(rules, description);
        }
        for (int j = 0; j < rulesJSONArray.length(); j++) {
            JSONObject ruleJSONObject = rulesJSONArray.getJSONObject(j);
            // Build conditions list
            List<BusinessLogicRuleCondition> ruleConditions =
                    extractRuleConditionList(ruleJSONObject);
            // Build actions list
            List<BusinessLogicRuleAction> ruleActions =
                    extractRuleActionList(ruleJSONObject);
            rules.add(new BusinessLogicRule(ruleConditions, ruleActions));
        }
        return new BusinessLogicRulesList(rules, description);
    }

    /* Extract all BusinessLogicRuleConditions from a JSON business logic rule */
    private static List<BusinessLogicRuleCondition> extractRuleConditionList(
            JSONObject ruleJSONObject) throws JSONException {
        List<BusinessLogicRuleCondition> ruleConditions = new ArrayList<>();
        // Rules do not require a condition, return empty list if no condition is found
        JSONArray ruleConditionsJSONArray = null;
        try {
            ruleConditionsJSONArray = ruleJSONObject.getJSONArray(RULE_CONDITIONS);
        } catch (JSONException e) {
            return ruleConditions; // no conditions for this rule, apply in all cases
        }
        for (int i = 0; i < ruleConditionsJSONArray.length(); i++) {
            JSONObject ruleConditionJSONObject = ruleConditionsJSONArray.getJSONObject(i);
            String methodName = ruleConditionJSONObject.getString(METHOD_NAME);
            boolean negated = false;
            if (methodName.startsWith("!")) {
                methodName = methodName.substring(1); // remove negation from method name string
                negated = true; // change "negated" property to true
            }
            List<String> methodArgs = new ArrayList<>();
            JSONArray methodArgsJSONArray = null;
            try {
                methodArgsJSONArray = ruleConditionJSONObject.getJSONArray(METHOD_ARGS);
            } catch (JSONException e) {
                // No method args for this rule condition, add rule condition with empty args list
                ruleConditions.add(new BusinessLogicRuleCondition(methodName, methodArgs, negated));
                continue;
            }
            for (int j = 0; j < methodArgsJSONArray.length(); j++) {
                methodArgs.add(methodArgsJSONArray.getString(j));
            }
            ruleConditions.add(new BusinessLogicRuleCondition(methodName, methodArgs, negated));
        }
        return ruleConditions;
    }

    /* Extract all BusinessLogicRuleActions from a JSON business logic rule */
    private static List<BusinessLogicRuleAction> extractRuleActionList(JSONObject ruleJSONObject)
            throws JSONException {
        List<BusinessLogicRuleAction> ruleActions = new ArrayList<>();
        // All rules require at least one action, line below throws JSONException if not
        JSONArray ruleActionsJSONArray = ruleJSONObject.getJSONArray(RULE_ACTIONS);
        for (int i = 0; i < ruleActionsJSONArray.length(); i++) {
            JSONObject ruleActionJSONObject = ruleActionsJSONArray.getJSONObject(i);
            String methodName = ruleActionJSONObject.getString(METHOD_NAME);
            List<String> methodArgs = new ArrayList<>();
            JSONArray methodArgsJSONArray = null;
            try {
                methodArgsJSONArray = ruleActionJSONObject.getJSONArray(METHOD_ARGS);
            } catch (JSONException e) {
                // No method args for this rule action, add rule action with empty args list
                ruleActions.add(new BusinessLogicRuleAction(methodName, methodArgs));
                continue;
            }
            for (int j = 0; j < methodArgsJSONArray.length(); j++) {
                methodArgs.add(methodArgsJSONArray.getString(j));
            }
            ruleActions.add(new BusinessLogicRuleAction(methodName, methodArgs));
        }
        return ruleActions;
    }

    /* Pare a timestamp string with format TIMESTAMP_PATTERN to a date object */
    private static Date parseTimestamp(String timestamp) {
        SimpleDateFormat format = new SimpleDateFormat(TIMESTAMP_PATTERN);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        try {
            return format.parse(timestamp);
        } catch (ParseException e) {
            return null;
        }
    }

    /* Extract string from file */
    private static String readFile(File f) throws IOException {
        StringBuilder sb = new StringBuilder((int) f.length());
        String lineSeparator = System.getProperty("line.separator");
        try (Scanner scanner = new Scanner(f)) {
            while(scanner.hasNextLine()) {
                sb.append(scanner.nextLine() + lineSeparator);
            }
            return sb.toString();
        }
    }
}
