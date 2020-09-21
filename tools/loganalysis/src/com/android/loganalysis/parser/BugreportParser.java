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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.ActivityServiceItem;
import com.android.loganalysis.item.AnrItem;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.BugreportItem.CommandLineItem;
import com.android.loganalysis.item.DumpsysItem;
import com.android.loganalysis.item.IItem;
import com.android.loganalysis.item.KernelLogItem;
import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.item.MemInfoItem;
import com.android.loganalysis.item.MiscKernelLogItem;
import com.android.loganalysis.item.MiscLogcatItem;
import com.android.loganalysis.item.ProcrankItem;
import com.android.loganalysis.item.SystemPropsItem;
import com.android.loganalysis.item.TopItem;
import com.android.loganalysis.item.TracesItem;

import java.io.BufferedReader;
import java.io.IOException;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.ListIterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse Android bugreports.
 */
public class BugreportParser extends AbstractSectionParser {
    private static final String MEM_INFO_SECTION_REGEX = "------ MEMORY INFO .*";
    private static final String PROCRANK_SECTION_REGEX = "------ PROCRANK .*";
    private static final String KERNEL_LOG_SECTION_REGEX = "------ KERNEL LOG .*";
    private static final String LAST_KMSG_SECTION_REGEX = "------ LAST KMSG .*";
    private static final String TOP_SECTION_REGEX = "------ CPU INFO .*";
    private static final String SYSTEM_PROP_SECTION_REGEX = "------ SYSTEM PROPERTIES .*";
    private static final String SYSTEM_LOG_SECTION_REGEX =
            "------ (SYSTEM|MAIN|MAIN AND SYSTEM) LOG .*";
    private static final String ANR_TRACES_SECTION_REGEX = "------ VM TRACES AT LAST ANR .*";
    private static final String DUMPSYS_SECTION_REGEX = "------ DUMPSYS .*";
    private static final String ACTIVITY_SERVICE_SECTION_REGEX =
            "^------ APP SERVICES \\(dumpsys activity service all\\) ------$";
    private static final String NOOP_SECTION_REGEX = "------ .* ------";

    private static final String BOOTREASON_PROP = "ro.boot.bootreason";
    private static final String BOOTREASON_KERNEL = "androidboot.bootreason";

    /**
     * Matches: == dumpstate: 2012-04-26 12:13:14
     */
    private static final Pattern DATE = Pattern.compile(
            "^== dumpstate: (\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})$");

    /**
     * Matches: Command line: key=value key=value
     */
    private static final Pattern COMMAND_LINE = Pattern.compile(
            "Command line: (.*)");

    private IParser mBugreportParser = new IParser() {
        @Override
        public BugreportItem parse(List<String> lines) {
            BugreportItem bugreport = null;
            for (String line : lines) {
                if (bugreport == null && !"".equals(line.trim())) {
                    bugreport = new BugreportItem();
                }
                Matcher m = DATE.matcher(line);
                if (m.matches()) {
                    bugreport.setTime(parseTime(m.group(1)));
                }
                m = COMMAND_LINE.matcher(line);
                if (m.matches()) {
                    String argString = m.group(1).trim();
                    if (!argString.isEmpty()) {
                        String[] args = argString.split("\\s+");
                        for (String arg : args) {
                            String[] keyValue = arg.split("=", 2);
                            if (keyValue.length == 2) {
                                mCommandLine.put(keyValue[0], keyValue[1]);
                            } else {
                                mCommandLine.put(keyValue[0], null);
                            }
                        }
                    }
                }
            }
            return bugreport;
        }
    };
    private MemInfoParser mMemInfoParser = new MemInfoParser();
    private ProcrankParser mProcrankParser = new ProcrankParser();
    private TopParser mTopParser = new TopParser();
    private SystemPropsParser mSystemPropsParser = new SystemPropsParser();
    private TracesParser mTracesParser = new TracesParser();
    private KernelLogParser mKernelLogParser = new KernelLogParser();
    private KernelLogParser mLastKmsgParser = new KernelLogParser();
    private LogcatParser mLogcatParser = new LogcatParser();
    private DumpsysParser mDumpsysParser = new DumpsysParser();
    private ActivityServiceParser mActivityServiceParser =  new ActivityServiceParser();

    private BugreportItem mBugreport = null;
    private CommandLineItem mCommandLine = new CommandLineItem();

    private boolean mParsedInput = false;

    /**
     * Parse a bugreport from a {@link BufferedReader} into an {@link BugreportItem} object.
     *
     * @param input a {@link BufferedReader}.
     * @return The {@link BugreportItem}.
     * @see #parse(List)
     */
    public BugreportItem parse(BufferedReader input) throws IOException {
        String line;

        setup();
        while ((line = input.readLine()) != null) {
            if (!mParsedInput && !"".equals(line.trim())) {
                mParsedInput = true;
            }
            parseLine(line);
        }
        commit();

        return mBugreport;
    }

    /**
     * {@inheritDoc}
     *
     * @return The {@link BugreportItem}.
     */
    @Override
    public BugreportItem parse(List<String> lines) {
        setup();
        for (String line : lines) {
            if (!mParsedInput && !"".equals(line.trim())) {
                mParsedInput = true;
            }
            parseLine(line);
        }
        commit();

        return mBugreport;
    }

    /**
     * Sets up the parser by adding the section parsers and adding an initial {@link IParser} to
     * parse the bugreport header.
     */
    protected void setup() {
        // Set the initial parser explicitly since the header isn't part of a section.
        setParser(mBugreportParser);
        addSectionParser(mMemInfoParser, MEM_INFO_SECTION_REGEX);
        addSectionParser(mProcrankParser, PROCRANK_SECTION_REGEX);
        addSectionParser(mTopParser, TOP_SECTION_REGEX);
        addSectionParser(mSystemPropsParser, SYSTEM_PROP_SECTION_REGEX);
        addSectionParser(mTracesParser, ANR_TRACES_SECTION_REGEX);
        addSectionParser(mLogcatParser, SYSTEM_LOG_SECTION_REGEX);
        addSectionParser(mKernelLogParser, KERNEL_LOG_SECTION_REGEX);
        addSectionParser(mLastKmsgParser, LAST_KMSG_SECTION_REGEX);
        addSectionParser(mDumpsysParser, DUMPSYS_SECTION_REGEX);
        addSectionParser(mActivityServiceParser, ACTIVITY_SERVICE_SECTION_REGEX);
        addSectionParser(new NoopParser(), NOOP_SECTION_REGEX);
        mKernelLogParser.setAddUnknownBootreason(false);
        mLastKmsgParser.setAddUnknownBootreason(false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void commit() {
        // signal EOF
        super.commit();

        if (mParsedInput && mBugreport == null) {
            mBugreport = new BugreportItem();
        }

        if (mBugreport != null) {
            mBugreport.setCommandLine(mCommandLine);
            mBugreport.setMemInfo((MemInfoItem) getSection(mMemInfoParser));
            mBugreport.setProcrank((ProcrankItem) getSection(mProcrankParser));
            mBugreport.setTop((TopItem) getSection(mTopParser));
            mBugreport.setSystemLog((LogcatItem) getSection(mLogcatParser));
            mBugreport.setKernelLog((KernelLogItem) getSection(mKernelLogParser));
            mBugreport.setLastKmsg((KernelLogItem) getSection(mLastKmsgParser));
            mBugreport.setSystemProps((SystemPropsItem) getSection(mSystemPropsParser));
            mBugreport.setDumpsys((DumpsysItem) getSection(mDumpsysParser));
            mBugreport.setActivityService((ActivityServiceItem) getSection(mActivityServiceParser));

            if (mBugreport.getSystemLog() != null && mBugreport.getProcrank() != null) {
                for (IItem item : mBugreport.getSystemLog().getEvents()) {
                    if (item instanceof MiscLogcatItem &&
                            ((MiscLogcatItem) item).getApp() == null) {
                        MiscLogcatItem logcatItem = (MiscLogcatItem) item;
                        logcatItem.setApp(mBugreport.getProcrank().getProcessName(
                                logcatItem.getPid()));
                    }
                }
            }

            TracesItem traces = (TracesItem) getSection(mTracesParser);
            if (traces != null && traces.getApp() != null && traces.getStack() != null &&
                    mBugreport.getSystemLog() != null) {
                addAnrTrace(mBugreport.getSystemLog().getAnrs(), traces.getApp(),
                        traces.getStack());
            }

            KernelLogItem lastKmsg = mBugreport.getLastKmsg();
            if (lastKmsg == null) {
                lastKmsg = new KernelLogItem();
                mBugreport.setLastKmsg(lastKmsg);
            }
            String bootreason = null;
            if (mBugreport.getSystemProps() != null &&
                    mBugreport.getSystemProps().containsKey(BOOTREASON_PROP)) {
                bootreason = mBugreport.getSystemProps().get(BOOTREASON_PROP);
            } else if (mCommandLine.containsKey(BOOTREASON_KERNEL)) {
                bootreason = mCommandLine.get(BOOTREASON_KERNEL);
            }
            if (bootreason != null) {
                Matcher m = KernelLogParser.BAD_BOOTREASONS.matcher(bootreason);
                if (m.matches()) {
                    MiscKernelLogItem item = new MiscKernelLogItem();
                    item.setStack("Last boot reason: " + bootreason.trim());
                    item.setCategory(KernelLogParser.KERNEL_RESET);
                    item.setPreamble("");
                    item.setEventTime(0.0);
                    lastKmsg.addEvent(item);
                }
                m = KernelLogParser.GOOD_BOOTREASONS.matcher(bootreason);
                if (m.matches()) {
                    MiscKernelLogItem item = new MiscKernelLogItem();
                    item.setStack("Last boot reason: " + bootreason.trim());
                    item.setCategory(KernelLogParser.NORMAL_REBOOT);
                    lastKmsg.addEvent(item);
                }
            }

            if (lastKmsg.getMiscEvents(KernelLogParser.KERNEL_RESET).isEmpty() &&
                    lastKmsg.getMiscEvents(KernelLogParser.NORMAL_REBOOT).isEmpty()) {
                MiscKernelLogItem unknownReset = new MiscKernelLogItem();
                unknownReset.setStack("Unknown reason");
                unknownReset.setCategory(KernelLogParser.KERNEL_RESET);
                unknownReset.setPreamble("");
                unknownReset.setEventTime(0.0);
                lastKmsg.addEvent(unknownReset);
            }
        }
    }

    /**
     * Add the trace from {@link TracesItem} to the last seen {@link AnrItem} matching a given app.
     */
    private void addAnrTrace(List<AnrItem> anrs, String app, String trace) {
        ListIterator<AnrItem> li = anrs.listIterator(anrs.size());

        while (li.hasPrevious()) {
            AnrItem anr = li.previous();
            if (app.equals(anr.getApp())) {
                anr.setTrace(trace);
                return;
            }
        }
    }

    /**
     * Set the {@link BugreportItem} and the year of the {@link LogcatParser} from the bugreport
     * header.
     */
    @Override
    protected void onSwitchParser() {
        if (mBugreport == null) {
            mBugreport = (BugreportItem) getSection(mBugreportParser);
            if (mBugreport != null && mBugreport.getTime() != null) {
                mLogcatParser.setYear(new SimpleDateFormat("yyyy").format(mBugreport.getTime()));
            }
        }
    }

    /**
     * Converts a {@link String} into a {@link Date}.
     */
    private static Date parseTime(String timeStr) {
        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        try {
            return formatter.parse(timeStr);
        } catch (ParseException e) {
            // CLog.e("Could not parse time string %s", timeStr);
            return null;
        }
    }
}

