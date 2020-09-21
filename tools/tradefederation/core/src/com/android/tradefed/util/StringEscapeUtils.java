/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.tradefed.util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Utility class for escaping strings for specific formats.
 * Include methods to escape strings that are being passed to the Android Shell.
 */
public class StringEscapeUtils {

    /**
     * Escapes a {@link String} for use in an Android shell command.
     *
     * @param str the {@link String} to escape
     * @return the Android shell escaped {@link String}
     */
    public static String escapeShell(String str) {
        if (str == null) {
            return null;
        }
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < str.length(); ++i) {
            char ch = str.charAt(i);
            // TODO: add other characters as needed.
            switch (ch) {
                case '$':
                    out.append("\\$");
                    break;
                case '\\':
                    out.append("\\\\");
                    break;
                default:
                    out.append(ch);
                    break;
            }
        }
        return out.toString();
    }

    /**
     * Converts the provided parameters via options to command line args to sub process
     *
     * <p>This method will do a simplistic generic unescape for each parameter in the list. It
     * replaces \[char] with [char]. For example, \" is converted to ". This allows string with
     * escaped double quotes to stay as a string after being parsed by QuotationAwareTokenizer.
     * Without this QuotationAwareTokenizer will break the string into sections if it has space in
     * it.
     *
     * @param params parameters received via options
     * @return list of string representing command line args
     */
    public static List<String> paramsToArgs(List<String> params) {
        List<String> result = new ArrayList<>();
        for (String param : params) {
            // doing a simplistic generic unescape here: \<char> is replaced with <char>; note that
            // this may lead to incorrect results such as \n -> n, or \t -> t, but it's unclear why
            // a command line param would have \t anyways
            param = param.replaceAll("\\\\(.)", "$1");
            String[] args = QuotationAwareTokenizer.tokenizeLine(param);
            if (args.length != 0) {
                result.addAll(Arrays.asList(args));
            }
        }
        return result;
    }
}
