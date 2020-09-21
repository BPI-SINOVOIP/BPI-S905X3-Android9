/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.loganalysis.item.DmesgActionInfoItem;
import com.android.loganalysis.item.DmesgItem;
import com.android.loganalysis.item.DmesgServiceInfoItem;
import com.android.loganalysis.item.DmesgStageInfoItem;

import com.google.common.annotations.VisibleForTesting;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * Parse the dmesg logs. </p>
 */
public class DmesgParser implements IParser {

    private static final String SERVICENAME = "SERVICENAME";
    private static final String TIMESTAMP = "TIMESTAMP";
    private static final String STAGE = "STAGE";
    private static final String ACTION = "ACTION";
    private static final String DURATION = "DURATION";
    private static final String UEVENTD = "ueventd";

    // Matches: [ 14.822691] init:
    private static final String SERVICE_PREFIX = String.format("^\\[\\s+(?<%s>.*)\\] init:\\s+",
            TIMESTAMP);
    // Matches: [    3.791635] ueventd:
    private static final String UEVENTD_PREFIX = String.format("^\\[\\s+(?<%s>.*)\\] ueventd:\\s+",
            TIMESTAMP);

    // Matches: starting service 'ueventd'...
    private static final String START_SERVICE_SUFFIX = String.format("starting service "
            + "\\'(?<%s>.*)\\'...", SERVICENAME);
    // Matches: Service 'ueventd' (pid 439) exited with status 0
    private static final String EXIT_SERVICE_SUFFIX = String.format("Service \\'(?<%s>.*)\\'\\s+"
            + "\\((?<PID>.*)\\) exited with status 0.*", SERVICENAME);

    private static final Pattern START_SERVICE = Pattern.compile(
            String.format("%s%s", SERVICE_PREFIX, START_SERVICE_SUFFIX));
    private static final Pattern EXIT_SERVICE = Pattern.compile(
            String.format("%s%s", SERVICE_PREFIX, EXIT_SERVICE_SUFFIX));

    // Matches: init first stage started!
    // Matches: init second stage started!
    private static final String START_STAGE_PREFIX = String.format("init (?<%s>.*) stage started!",
            STAGE);

    // Matches: [   13.647997] init: init first stage started!
    private static final Pattern START_STAGE = Pattern.compile(
            String.format("%s%s", SERVICE_PREFIX, START_STAGE_PREFIX));

    // Matches: init: processing action (early-init) from (/init.rc:14)
    private static final String START_PROCESSING_ACTION_PREFIX =
            String.format("processing action \\((?<%s>.*)\\) from.*$", ACTION);

    // Matches: init: processing action (early-init)
    private static final String START_PROCESSING_ACTION_PREFIX_LEGACY =
            String.format("processing action \\((?<%s>.*)\\).*$", ACTION);

    // Matches: init: processing action (early-init) from (/init.rc:14)
    private static final Pattern START_PROCESSING_ACTION =
            Pattern.compile(String.format("%s%s", SERVICE_PREFIX, START_PROCESSING_ACTION_PREFIX));

    // Matches: init: processing action (early-init)
    private static final Pattern START_PROCESSING_ACTION_LEGACY =
            Pattern.compile(
                    String.format("%s%s", SERVICE_PREFIX, START_PROCESSING_ACTION_PREFIX_LEGACY));

    // Matches: [    3.791635] ueventd: Coldboot took 0.695055 seconds
    private static final String STAGE_SUFFIX= String.format(
            "(?<%s>.*)\\s+took\\s+(?<%s>.*)\\s+seconds$", STAGE, DURATION);
    private static final Pattern UEVENTD_STAGE_INFO = Pattern.compile(
            String.format("%s%s", UEVENTD_PREFIX, STAGE_SUFFIX));


    private DmesgItem mDmesgItem = new DmesgItem();

    /**
     * {@inheritDoc}
     */
    @Override
    public DmesgItem parse(List<String> lines) {
        for (String line : lines) {
            parse(line);
        }

        return mDmesgItem;
    }

    /**
     * Parse the kernel log till EOF to retrieve the duration of the service calls, start times of
     * different boot stages and actions taken. Besides, while parsing these informations are stored
     * in the intermediate {@link DmesgServiceInfoItem}, {@link DmesgStageInfoItem} and
     * {@link DmesgActionInfoItem} objects
     *
     * @param input dmesg log
     * @throws IOException
     */
    public DmesgItem parseInfo(BufferedReader bufferedLog) throws IOException {
        String line;
        while ((line = bufferedLog.readLine()) != null) {
            parse(line);
        }

        return mDmesgItem;
    }

    /**
     * <p>
     * Parse single line of the dmesg log to retrieve the duration of the service calls or start
     * times of different boot stages or start times of actions taken based on the info contained in
     * the line.
     * </p>
     * <p>
     * Besides, while parsing these informations are stored in the intermediate
     * {@link DmesgServiceInfoItem}, {@link DmesgStageInfoItem} and {@link DmesgActionInfoItem}
     * objects
     * </p>
     *
     * @param line individual line of the dmesg log
     */
    private void parse(String line) {
        if (parseServiceInfo(line)) {
            return;
        }
        if (parseStageInfo(line)) {
            return;
        }
        if (parseActionInfo(line)) {
            return;
        }
    }

    /**
     * Parse init services {@code start time} and {@code end time} from each {@code line} of dmesg
     * log and store the {@code duration} it took to complete the service if the end time stamp is
     * available in {@link DmesgServiceInfoItem}.
     *
     * @param individual line of the dmesg log
     * @return {@code true}, if the {@code line} indicates start or end of a service,
     *         {@code false}, otherwise
     */
    @VisibleForTesting
    boolean parseServiceInfo(String line) {
        Matcher match = null;
        if ((match = matches(START_SERVICE, line)) != null) {
            DmesgServiceInfoItem serviceItem = new DmesgServiceInfoItem();
            serviceItem.setServiceName(match.group(SERVICENAME));
            serviceItem.setStartTime((long) (Double.parseDouble(
                    match.group(TIMESTAMP)) * 1000));
            getServiceInfoItems().put(match.group(SERVICENAME), serviceItem);
            return true;
        } else if ((match = matches(EXIT_SERVICE, line)) != null) {
            if (getServiceInfoItems().containsKey(match.group(SERVICENAME))) {
                DmesgServiceInfoItem serviceItem = getServiceInfoItems().get(
                        match.group(SERVICENAME));
                serviceItem.setEndTime((long) (Double.parseDouble(
                        match.group(TIMESTAMP)) * 1000));
            }
            return true;
        }
        return false;
    }

    /**
     * Parse init stages log from each {@code line} of dmesg log and
     * store the stage name, start time and duration if available in a
     * {@link DmesgStageInfoItem} object
     *
     * @param individual line of the dmesg log
     * @return {@code true}, if the {@code line} indicates start of a boot stage,
     *         {@code false}, otherwise
     */
    @VisibleForTesting
    boolean parseStageInfo(String line) {
        Matcher match = null;
        if ((match = matches(START_STAGE, line)) != null) {
            DmesgStageInfoItem stageInfoItem = new DmesgStageInfoItem();
            stageInfoItem.setStageName(match.group(STAGE));
            stageInfoItem.setStartTime((long) (Double.parseDouble(
                    match.group(TIMESTAMP)) * 1000));
            mDmesgItem.addStageInfoItem(stageInfoItem);
            return true;
        }
        if((match = matches(UEVENTD_STAGE_INFO, line)) != null) {
            DmesgStageInfoItem stageInfoItem = new DmesgStageInfoItem();
            stageInfoItem.setStageName(String.format("%s_%s", UEVENTD, match.group(STAGE)));
            stageInfoItem.setDuration((long) (Double.parseDouble(
                    match.group(DURATION)) * 1000));
            mDmesgItem.addStageInfoItem(stageInfoItem);
        }
        return false;
    }

    /**
     * Parse action from each {@code line} of dmesg log and store the action name and start time
     * in {@link DmesgActionInfoItem} object
     *
     * @param individual {@code line} of the dmesg log
     * @return {@code true}, if {@code line} indicates starting of processing of action
     *         {@code false}, otherwise
     */
    @VisibleForTesting
    boolean parseActionInfo(String line) {
        Matcher match = null;
        if ((match = matches(START_PROCESSING_ACTION, line)) != null
                || (match = matches(START_PROCESSING_ACTION_LEGACY, line)) != null) {
            DmesgActionInfoItem actionInfoItem = new DmesgActionInfoItem();
            actionInfoItem.setActionName(match.group(ACTION));
            actionInfoItem.setStartTime((long) (Double.parseDouble(
                    match.group(TIMESTAMP)) * 1000));
            mDmesgItem.addActionInfoItem(actionInfoItem);
            return true;
        }
        return false;
    }

    /**
     * Checks whether {@code line} matches the given {@link Pattern}.
     *
     * @return The resulting {@link Matcher} obtained by matching the {@code line} against
     *         {@code pattern}, or null if the {@code line} does not match.
     */
    private static Matcher matches(Pattern pattern, String line) {
        Matcher ret = pattern.matcher(line);
        return ret.matches() ? ret : null;
    }

    public Map<String, DmesgServiceInfoItem> getServiceInfoItems() {
        return mDmesgItem.getServiceInfoItems();
    }

    public void setServiceInfoItems(Map<String, DmesgServiceInfoItem> serviceInfoItems) {
        for(String key : serviceInfoItems.keySet()) {
            mDmesgItem.addServiceInfoItem(key, serviceInfoItems.get(key));
        }
    }

    public List<DmesgStageInfoItem> getStageInfoItems() {
        return mDmesgItem.getStageInfoItems();
    }

    public List<DmesgActionInfoItem> getActionInfoItems() {
        return mDmesgItem.getActionInfoItems();
    }

}
