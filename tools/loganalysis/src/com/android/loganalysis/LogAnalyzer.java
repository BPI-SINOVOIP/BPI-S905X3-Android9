/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.loganalysis;

import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.DvmLockSampleItem;
import com.android.loganalysis.item.IItem;
import com.android.loganalysis.item.KernelLogItem;
import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.item.MemoryHealthItem;
import com.android.loganalysis.item.MonkeyLogItem;
import com.android.loganalysis.parser.BugreportParser;
import com.android.loganalysis.parser.DvmLockSampleParser;
import com.android.loganalysis.parser.KernelLogParser;
import com.android.loganalysis.parser.LogcatParser;
import com.android.loganalysis.parser.MemoryHealthParser;
import com.android.loganalysis.parser.MonkeyLogParser;
import com.android.loganalysis.rule.RuleEngine;
import com.android.loganalysis.rule.RuleEngine.RuleType;
import com.android.loganalysis.util.config.ArgsOptionParser;
import com.android.loganalysis.util.config.ConfigurationException;
import com.android.loganalysis.util.config.Option;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import java.util.ArrayList;
import java.util.List;

/**
 * A command line tool to parse a bugreport, logcat, or kernel log file and return the output.
 */
public class LogAnalyzer {

    private enum OutputFormat{
        // TODO: Add text output support.
        JSON;
    }

    private enum ResultType {
        RAW, ANALYSIS;
    }

    @Option(name="bugreport", description="The path to the bugreport")
    private String mBugreportPath = null;

    @Option(name="logcat", description="The path to the logcat")
    private String mLogcatPath = null;

    @Option(name="kernel-log", description="The path to the kernel log")
    private String mKernelLogPath = null;

    @Option(name="monkey-log", description="The path to the monkey log")
    private String mMonkeyLogPath = null;

    @Option(name="memory-health", description="The path to the memory health log")
    private String mMemoryHealthLogPath = null;

    @Option(name="output", description="The output format, currently only JSON")
    private OutputFormat mOutputFormat = OutputFormat.JSON;

    @Option(name="rule-type", description="The type of rules to be applied")
    private RuleType mRuleType = RuleType.ALL;

    @Option(name="print", description="Print the result type")
    private List<ResultType> mResultType = new ArrayList<ResultType>();

    @Option(name="events-log", description="The path to the events log")
    private String mEventsLogPath = null;

    /** Constant for JSON output */
    private static final String RAW_DATA = "RAW";
    /** Constant for JSON output */
    private static final String ANALYSIS_DATA = "ANALYSIS";

    /**
     * Run the command line tool
     */
    public void run(String[] args) {
        try {
            initArgs(args);
        } catch (ConfigurationException e) {
            printUsage();
            return;
        }

        if (!checkPreconditions()) {
            printUsage();
            return;
        }

        BufferedReader reader = null;
        try {
            if (mBugreportPath != null) {
                reader = getBufferedReader(mBugreportPath);
                BugreportItem bugreport = new BugreportParser().parse(reader);
                printBugreport(bugreport);
                return;
            }

            if (mLogcatPath != null) {
                reader = getBufferedReader(mLogcatPath);
                LogcatItem logcat = new LogcatParser().parse(reader);
                printLogcat(logcat);
                return;
            }

            if (mKernelLogPath != null) {
                reader = getBufferedReader(mKernelLogPath);
                KernelLogItem kernelLog = new KernelLogParser().parse(reader);
                printKernelLog(kernelLog);
                return;
            }

            if (mMonkeyLogPath != null) {
                reader = getBufferedReader(mMonkeyLogPath);
                MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(reader);
                printMonkeyLog(monkeyLog);
                return;
            }

            if (mMemoryHealthLogPath != null) {
                reader = getBufferedReader(mMemoryHealthLogPath);
                MemoryHealthItem item = new MemoryHealthParser().parse(reader);
                printMemoryHealthLog(item);
                return;
            }

            if (mEventsLogPath != null) {
                reader = getBufferedReader(mEventsLogPath);

                // The only log we know how to parse in the Events log are
                // DVM lock samples.
                DvmLockSampleItem item = new DvmLockSampleParser().parse(reader);
                printDVMLog(item);
                return;
            }
        } catch (FileNotFoundException e) {
            System.err.println(e.getMessage());
        } catch (IOException e) {
            System.err.println(e.getMessage());
        } finally {
            close(reader);
        }

        // Should never reach here.
        printUsage();
    }

    private void printMemoryHealthLog(MemoryHealthItem item) {
        System.out.println(item.toJson().toString());
    }

    /**
     * Print the bugreport to stdout.
     */
    private void printBugreport(BugreportItem bugreport) {
        if (OutputFormat.JSON.equals(mOutputFormat)) {
            if (mResultType.size() == 0) {
                printJson(bugreport);
            } else if (mResultType.size() == 1) {
                switch (mResultType.get(0)) {
                    case RAW:
                        printJson(bugreport);
                        break;
                    case ANALYSIS:
                        printBugreportAnalysis(getBugreportAnalysis(bugreport));
                        break;
                    default:
                        // should not get here
                        return;
                }
            } else {
                JSONObject result = new JSONObject();
                try {
                    for (ResultType resultType : mResultType) {
                        switch (resultType) {
                            case RAW:
                                result.put(RAW_DATA, bugreport.toJson());
                                break;
                            case ANALYSIS:
                                result.put(ANALYSIS_DATA, getBugreportAnalysis(bugreport));
                                break;
                            default:
                                // should not get here
                                break;
                        }
                    }
                } catch (JSONException e) {
                    // Ignore
                }
                printJson(result);
            }
        }
    }

    private JSONArray getBugreportAnalysis(BugreportItem bugreport) {
        RuleEngine ruleEngine = new RuleEngine(bugreport);
        ruleEngine.registerRules(mRuleType);
        ruleEngine.executeRules();
        if (ruleEngine.getAnalysis() != null) {
            return ruleEngine.getAnalysis();
        } else {
            return new JSONArray();
        }
    }

    private void printBugreportAnalysis(JSONArray analysis) {
        if (analysis != null && analysis.length() > 0) {
            System.out.println(analysis.toString());
        } else {
            System.out.println(new JSONObject().toString());
        }
    }

    /**
     * Print the logcat to stdout.
     */
    private void printLogcat(LogcatItem logcat) {
        if (OutputFormat.JSON.equals(mOutputFormat)) {
            printJson(logcat);
        }
        // TODO: Print logcat in human readable form.
    }

    /**
     * Print the kernel log to stdout.
     */
    private void printKernelLog(KernelLogItem kernelLog) {
        if (OutputFormat.JSON.equals(mOutputFormat)) {
            printJson(kernelLog);
        }
        // TODO: Print kernel log in human readable form.
    }

    /**
     * Print the monkey log to stdout.
     */
    private void printMonkeyLog(MonkeyLogItem monkeyLog) {
        if (OutputFormat.JSON.equals(mOutputFormat)) {
            printJson(monkeyLog);
        }
        // TODO: Print monkey log in human readable form.
    }

    /**
     * Print a DVM log entry to stdout.
     */
    private void printDVMLog(DvmLockSampleItem dvmLog) {
        if (OutputFormat.JSON.equals(mOutputFormat)) {
            printJson(dvmLog);
        }
        // TODO: Print DVM log in human readable form.
    }

    /**
     * Print an {@link IItem} to stdout.
     */
    private void printJson(IItem item) {
        if (item != null && item.toJson() != null) {
            printJson(item.toJson());
        } else {
            printJson(new JSONObject());
        }
    }

    /**
     * Print an {@link JSONObject} to stdout
     */
    private void printJson(JSONObject json) {
        if (json != null) {
            System.out.println(json.toString());
        } else {
            System.out.println(new JSONObject().toString());
        }
    }

    /**
     * Get a {@link BufferedReader} from a given filepath.
     * @param filepath the path to the file.
     * @return The {@link BufferedReader} containing the contents of the file.
     * @throws FileNotFoundException if the file could not be found.
     */
    private BufferedReader getBufferedReader(String filepath) throws FileNotFoundException {
        return new BufferedReader(new FileReader(new File(filepath)));
    }

    /**
     * Helper to close a {@link Closeable}.
     */
    private void close(Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }

    /**
     * Parse the command line options and set {@link Option} annotated fields.
     */
    private void initArgs(String[] args) throws ConfigurationException {
        ArgsOptionParser opt = new ArgsOptionParser(this);
        opt.parse(args);
    }

    /**
     * Checks the arguments to see if they are valid.
     *
     * @return true if they are valid, false if they are not.
     */
    private boolean checkPreconditions() {
        // Check to see that exactly one log is set.
        int logCount = 0;
        if (mBugreportPath != null) logCount++;
        if (mLogcatPath != null) logCount++;
        if (mKernelLogPath != null) logCount++;
        if (mMonkeyLogPath != null) logCount++;
        if (mMemoryHealthLogPath != null) logCount++;
        return (logCount == 1);
    }

    /**
     * Print the usage for the command.
     */
    private void printUsage() {
        System.err.println("Usage: loganalysis [--bugreport FILE | --events-log FILE | --logcat FILE | " +
                "--kernel-log FILE | --monkey-log FILE]");
    }

    /**
     * Run the LogAnalyzer from the command line.
     */
    public static void main(String[] args) {
        LogAnalyzer analyzer = new LogAnalyzer();
        analyzer.run(args);
    }
}
