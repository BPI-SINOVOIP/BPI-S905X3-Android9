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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.TraceFormatItem;

import com.google.common.base.CaseFormat;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Read trace format and generate a regex that matches output of such format.
 *
 * <p>Traces under /d/tracing specify the output format with a printf string. This parser reads such
 * string, finds all parameters, and generates a regex that matches output of such format. Each
 * parameter corresponds to a named-capturing group in the regex. The parameter names are converted
 * to camel case because Java regex group name must contain only letters and numbers.
 *
 * <p>An end-to-end example:
 *
 * <pre>{@code
 * List<String> formatLine = Arrays.asList("print fmt: \"foo=%llu, bar:%s\", REC->foo, REC->bar");
 * TraceFormatItem parsedFormat = new TraceFormatParser.parse(formatLine);
 * parsedFormat.getParameters(); // "foo", "bar"
 * parsedFormat.getNumericParameters(); // "foo"
 * Matcher matcher = parsedFormat.getRegex.matcher("foo=123, bar:enabled");
 * matcher.matches();
 * matcher.group("foo") // 123
 * matcher.group("bar") // "enabled"
 * }</pre>
 *
 * <p>The initial implementation supports some commonly used specifiers: signed and unsigned integer
 * (with or without long or long long modifier), floating point number (with or without precision),
 * hexadecimal number (with or without 0's padding), and string (contains only [a-zA-Z_0-9]). It is
 * assumed no characters found in the format line need to be escaped.
 *
 * <p>Some examples of trace format line:
 *
 * <p>(thermal/tsens_read)
 *
 * <p>print fmt: "temp=%lu sensor=tsens_tz_sensor%u", REC->temp, REC->sensor
 *
 * <p>(sched/sched_cpu_hotplug)
 *
 * <p>print fmt: "cpu %d %s error=%d", REC->affected_cpu, REC->status ? "online" : "offline",
 * REC->error
 *
 * <p>(mmc/mmc_blk_erase_start)
 *
 * <p>print fmt: "cmd=%u,addr=0x%08x,size=0x%08x", REC->cmd, REC->addr, REC->size
 */
public class TraceFormatParser implements IParser {
    // split the raw format line
    private static final Pattern SPLIT_FORMAT_LINE =
            Pattern.compile(".*?\"(?<printf>.*?)\"(?<params>.*)");
    // match parameter names
    private static final Pattern SPLIT_PARAMS = Pattern.compile("->(?<param>\\w+)");
    // match and categorize common printf specifiers
    // use ?: to flag all non-capturing group so any group captured correspond to a specifier
    private static final Pattern PRINTF_SPECIFIERS =
            Pattern.compile(
                    "(?<num>%(?:llu|lu|u|lld|ld|d|(?:.\\d*)?f))|(?<hex>%\\d*(?:x|X))|(?<str>%s)");

    // regex building blocks to match simple numeric/hex/string parameters, exposed for unit testing
    static final String MATCH_NUM = "-?\\\\d+(?:\\\\.\\\\d+)?";
    static final String MATCH_HEX = "[\\\\da-fA-F]+";
    static final String MATCH_STR = "[\\\\w]*";

    /** Parse a trace format line and return an {@link TraceFormatItem} */
    @Override
    public TraceFormatItem parse(List<String> lines) {
        // sanity check
        if (lines == null || lines.size() != 1) {
            throw new RuntimeException("Cannot parse format line: expect one-line trace format");
        }

        // split the raw format line
        Matcher formatLineMatcher = SPLIT_FORMAT_LINE.matcher(lines.get(0));
        if (!formatLineMatcher.matches()) {
            throw new RuntimeException("Cannot parse format line: unexpected format");
        }
        String printfString = formatLineMatcher.group("printf");
        String paramsString = formatLineMatcher.group("params");

        // list of parameters, to be populated soon
        List<String> allParams = new ArrayList<>();
        List<String> numParams = new ArrayList<>();
        List<String> hexParams = new ArrayList<>();
        List<String> strParams = new ArrayList<>();

        // find all parameters and convert them to camel case
        Matcher paramsMatcher = SPLIT_PARAMS.matcher(paramsString);
        while (paramsMatcher.find()) {
            String camelCasedParam =
                    CaseFormat.LOWER_UNDERSCORE.to(
                            CaseFormat.LOWER_CAMEL, paramsMatcher.group("param"));
            allParams.add(camelCasedParam);
        }

        // scan the printf string, categorizing parameters and generating a matching regex
        StringBuffer regexBuilder = new StringBuffer();
        int paramIndex = 0;
        String currentParam;

        Matcher printfMatcher = PRINTF_SPECIFIERS.matcher(printfString);
        while (printfMatcher.find()) {
            // parameter corresponds to the found specifier
            currentParam = allParams.get(paramIndex++);
            if (printfMatcher.group("num") != null) {
                printfMatcher.appendReplacement(
                        regexBuilder, createNamedRegexGroup(MATCH_NUM, currentParam));
                numParams.add(currentParam);
            } else if (printfMatcher.group("hex") != null) {
                printfMatcher.appendReplacement(
                        regexBuilder, createNamedRegexGroup(MATCH_HEX, currentParam));
                hexParams.add(currentParam);
            } else if (printfMatcher.group("str") != null) {
                printfMatcher.appendReplacement(
                        regexBuilder, createNamedRegexGroup(MATCH_STR, currentParam));
                strParams.add(currentParam);
            } else {
                throw new RuntimeException("Unrecognized specifier: " + printfMatcher.group());
            }
        }
        printfMatcher.appendTail(regexBuilder);
        Pattern generatedRegex = Pattern.compile(regexBuilder.toString());

        // assemble and return a TraceFormatItem
        TraceFormatItem item = new TraceFormatItem();
        item.setRegex(generatedRegex);
        item.setParameters(allParams);
        item.setNumericParameters(numParams);
        item.setHexParameters(hexParams);
        item.setStringParameters(strParams);
        return item;
    }

    /** Helper function to create a regex group with given name. */
    private static String createNamedRegexGroup(String base, String name) {
        return String.format("(?<%s>%s)", name, base);
    }
}
