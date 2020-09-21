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
package com.android.loganalysis.item;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Pattern;

/** A {@link GenericItem} of trace format. */
public class TraceFormatItem extends GenericItem {
    private static final String REGEX = "regex";
    private static final String PARAMS = "params";
    private static final String NUM_PARAMS = "num_params";
    private static final String HEX_PARAMS = "hex_params";
    private static final String STR_PARAMS = "str_params";
    private static final Set<String> ATTRIBUTES =
            new HashSet<>(Arrays.asList(REGEX, PARAMS, NUM_PARAMS, HEX_PARAMS, STR_PARAMS));

    /** Create a new {@link TraceFormatItem} */
    public TraceFormatItem() {
        super(ATTRIBUTES);
    }

    @Override
    /** TraceFormatItem doesn't support merge */
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Trace format items cannot be merged");
    }

    /** Get a compiled regex that matches output of this trace format */
    public Pattern getRegex() {
        return (Pattern) getAttribute(REGEX);
    }

    /** Set a compiled regex that matches output of this trace format */
    public void setRegex(Pattern regex) {
        setAttribute(REGEX, regex);
    }

    /**
     * Get all parameters found in this trace format. The parameters were converted to camel case
     * and match the group names in the regex.
     */
    public List<String> getParameters() {
        return (List<String>) getAttribute(PARAMS);
    }

    /**
     * Set all parameters found in this trace format. The parameters were converted to camel case
     * and match the group names in the regex.
     */
    public void setParameters(List<String> parameters) {
        setAttribute(PARAMS, parameters);
    }

    /**
     * Get numeric parameters found in this trace format. The parameters were converted to camel
     * case and match the group names in the regex.
     */
    public List<String> getNumericParameters() {
        return (List<String>) getAttribute(NUM_PARAMS);
    }

    /**
     * Set numeric parameters found in this trace format. The parameters were converted to camel
     * case and match the group names in the regex.
     */
    public void setNumericParameters(List<String> numericParameters) {
        setAttribute(NUM_PARAMS, numericParameters);
    }

    /**
     * Get hexadecimal parameters found in this trace format. The parameters were converted to camel
     * case and match the group names in the regex.
     */
    public List<String> getHexParameters() {
        return (List<String>) getAttribute(HEX_PARAMS);
    }

    /**
     * Set hexadecimal parameters found in this trace format. The parameters were converted to camel
     * case and match the group names in the regex.
     */
    public void setHexParameters(List<String> hexParameters) {
        setAttribute(HEX_PARAMS, hexParameters);
    }

    /**
     * Get string parameters found in this trace format. The parameters were converted to camel case
     * and match the group names in the regex.
     */
    public List<String> getStringParameters() {
        return (List<String>) getAttribute(STR_PARAMS);
    }

    /**
     * Set string parameters found in this trace format. The parameters were converted to camel case
     * and match the group names in the regex.
     */
    public void setStringParameters(List<String> stringParameters) {
        setAttribute(STR_PARAMS, stringParameters);
    }
}
