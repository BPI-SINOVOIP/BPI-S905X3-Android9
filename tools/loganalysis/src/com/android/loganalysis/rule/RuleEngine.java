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
package com.android.loganalysis.rule;

import com.android.loganalysis.item.BugreportItem;

import org.json.JSONArray;

import java.util.Collection;
import java.util.LinkedList;


/**
 * Applies rules to the parsed bugreport
 */
public class RuleEngine {

    public enum RuleType{
        ALL, POWER;
    }

    BugreportItem mBugreportItem;
    private Collection<IRule> mRulesList;

    public RuleEngine(BugreportItem bugreportItem) {
        mBugreportItem = bugreportItem;
        mRulesList = new LinkedList<IRule>();
    }

    public void registerRules(RuleType ruleType) {
        if (ruleType == RuleType.ALL) {
            // add all rules
            addPowerRules();
        } else if (ruleType == RuleType.POWER) {
            addPowerRules();
        }
    }

    public void executeRules() {
        for (IRule rule : mRulesList) {
            rule.applyRule();
        }
    }

    public JSONArray getAnalysis() {
        JSONArray result = new JSONArray();
        for (IRule rule : mRulesList) {
            result.put(rule.getAnalysis());
        }
        return result;
    }

    private void addPowerRules() {
        mRulesList.add(new WakelockRule(mBugreportItem));
        mRulesList.add(new ProcessUsageRule(mBugreportItem));
        mRulesList.add(new LocationUsageRule(mBugreportItem));
        mRulesList.add(new WifiStatsRule(mBugreportItem));
        mRulesList.add(new InterruptRule(mBugreportItem));
    }
}
