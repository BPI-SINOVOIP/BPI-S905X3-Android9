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
package com.android.tradefed.util;

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.testtype.suite.ModuleDefinition;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Helper to serialize/deserialize the events to be passed to the log.
 */
public class SubprocessEventHelper {
    private static final String CLASSNAME_KEY = "className";
    private static final String TESTNAME_KEY = "testName";
    private static final String TRACE_KEY = "trace";
    private static final String CAUSE_KEY = "cause";
    private static final String RUNNAME_KEY = "runName";
    private static final String TESTCOUNT_KEY = "testCount";
    private static final String TIME_KEY = "time";
    private static final String REASON_KEY = "reason";
    private static final String START_TIME = "start_time";
    private static final String END_TIME = "end_time";

    private static final String DATA_NAME_KEY = "dataName";
    private static final String DATA_TYPE_KEY = "dataType";
    private static final String DATA_FILE_KEY = "dataFile";
    private static final String LOGGED_FILE_KEY = "loggedFile";

    private static final String TEST_TAG_KEY = "testTag";

    private static final String MODULE_CONTEXT_KEY = "moduleContextFileName";
    private static final String MODULE_NAME = "moduleName";

    /**
     * Helper for testRunStarted information
     */
    public static class TestRunStartedEventInfo {
        public String mRunName = null;
        public Integer mTestCount = null;

        public TestRunStartedEventInfo(String runName, int testCount) {
            mRunName = runName;
            mTestCount = testCount;
        }

        public TestRunStartedEventInfo(JSONObject jsonObject) throws JSONException {
            mRunName = jsonObject.getString(RUNNAME_KEY);
            mTestCount = jsonObject.getInt(TESTCOUNT_KEY);
        }

        @Override
        public String toString() {
            JSONObject tags = new JSONObject();
            try {
                if (mRunName != null) {
                    tags.put(RUNNAME_KEY, mRunName);
                }
                if (mTestCount != null) {
                    tags.put(TESTCOUNT_KEY, mTestCount.intValue());
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /**
     * Helper for testRunFailed information
     */
    public static class TestRunFailedEventInfo {
        public String mReason = null;

        public TestRunFailedEventInfo(String reason) {
            mReason = reason;
        }

        public TestRunFailedEventInfo(JSONObject jsonObject) throws JSONException {
            mReason = jsonObject.getString(REASON_KEY);
        }

        @Override
        public String toString() {
            JSONObject tags = new JSONObject();
            try {
                if (mReason != null) {
                    tags.put(REASON_KEY, mReason);
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /**
     * Helper for testRunEnded Information.
     */
    public static class TestRunEndedEventInfo {
        public Long mTime = null;
        public Map<String, String> mRunMetrics = null;

        public TestRunEndedEventInfo(Long time, Map<String, String> runMetrics) {
            mTime = time;
            mRunMetrics = runMetrics;
        }

        public TestRunEndedEventInfo(JSONObject jsonObject) throws JSONException {
            mTime = jsonObject.getLong(TIME_KEY);
            jsonObject.remove(TIME_KEY);
            Iterator<?> i = jsonObject.keys();
            mRunMetrics = new HashMap<String, String>();
            while(i.hasNext()) {
                String key = (String) i.next();
                mRunMetrics.put(key, jsonObject.get(key).toString());
            }
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                if (mRunMetrics != null) {
                    tags = new JSONObject(mRunMetrics);
                } else {
                    tags = new JSONObject();
                }
                if (mTime != null) {
                    tags.put(TIME_KEY, mTime.longValue());
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /**
     * Helper for InvocationFailed information.
     */
    public static class InvocationFailedEventInfo {
        public Throwable mCause = null;

        public InvocationFailedEventInfo(Throwable cause) {
            mCause = cause;
        }

        public InvocationFailedEventInfo(JSONObject jsonObject) throws JSONException {
            String stack = jsonObject.getString("cause");
            mCause = new Throwable(stack);
        }

        @Override
        public String toString() {
            JSONObject tags = new JSONObject();
            try {
                if (mCause != null) {
                    StringWriter sw = new StringWriter();
                    PrintWriter pw = new PrintWriter(sw);
                    mCause.printStackTrace(pw);
                    tags.put(CAUSE_KEY, sw.toString());
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /** Base Helper for TestIgnored information. */
    public static class BaseTestEventInfo {
        public String mClassName = null;
        public String mTestName = null;

        public BaseTestEventInfo(String className, String testName) {
            mClassName = className;
            mTestName = testName;
        }

        public BaseTestEventInfo(JSONObject jsonObject) throws JSONException {
            mClassName = jsonObject.getString(CLASSNAME_KEY);
            jsonObject.remove(CLASSNAME_KEY);
            mTestName = jsonObject.getString(TESTNAME_KEY);
            jsonObject.remove(TESTNAME_KEY);
        }

        protected JSONObject getNewJson() {
            return new JSONObject();
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                tags = getNewJson();
                if (mClassName != null) {
                    tags.put(CLASSNAME_KEY, mClassName);
                }
                if (mTestName != null) {
                    tags.put(TESTNAME_KEY, mTestName);
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /** Helper for testStarted information */
    public static class TestStartedEventInfo extends BaseTestEventInfo {
        public Long mStartTime = null;

        public TestStartedEventInfo(String className, String testName, Long startTime) {
            super(className, testName);
            mStartTime = startTime;
        }

        public TestStartedEventInfo(JSONObject jsonObject) throws JSONException {
            super(jsonObject);
            if (jsonObject.has(START_TIME)) {
                mStartTime = jsonObject.getLong(START_TIME);
            }
            jsonObject.remove(START_TIME);
        }

        @Override
        protected JSONObject getNewJson() {
            JSONObject json = new JSONObject();
            try {
                json.put(START_TIME, mStartTime);
            } catch (JSONException e) {
                CLog.e(e);
            }
            return json;
        }
    }

    /** Helper for testFailed information. */
    public static class FailedTestEventInfo extends BaseTestEventInfo {
        public String mTrace = null;

        public FailedTestEventInfo(String className, String testName, String trace) {
            super(className, testName);
            mTrace = trace;
        }

        public FailedTestEventInfo(JSONObject jsonObject) throws JSONException {
            super(jsonObject);
            mTrace = jsonObject.getString(TRACE_KEY);
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                tags = new JSONObject(super.toString());
                if (mTrace != null) {
                    tags.put(TRACE_KEY, mTrace);
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /**
     * Helper for testEnded information.
     */
    public static class TestEndedEventInfo extends BaseTestEventInfo {
        public Map<String, String> mRunMetrics = null;
        public Long mEndTime = null;

        public TestEndedEventInfo(String className, String testName,
                Map<String, String> runMetrics) {
            super(className, testName);
            mRunMetrics = runMetrics;
            mEndTime = System.currentTimeMillis();
        }

        /**
         * Create an event object to represent the testEnded callback.
         *
         * @param className the classname of the tests
         * @param testName the name of the tests
         * @param endTime the timestamp at which the test ended (from {@link
         *     System#currentTimeMillis()})
         * @param runMetrics the metrics reported by the test.
         */
        public TestEndedEventInfo(
                String className, String testName, Long endTime, Map<String, String> runMetrics) {
            super(className, testName);
            mEndTime = endTime;
            mRunMetrics = runMetrics;
        }

        /** Create and populate and event object for testEnded from a JSON. */
        public TestEndedEventInfo(JSONObject jsonObject) throws JSONException {
            super(jsonObject);
            if (jsonObject.has(END_TIME)) {
                mEndTime = jsonObject.getLong(END_TIME);
            }
            jsonObject.remove(END_TIME);
            Iterator<?> i = jsonObject.keys();
            mRunMetrics = new HashMap<String, String>();
            while(i.hasNext()) {
                String key = (String) i.next();
                mRunMetrics.put(key, jsonObject.get(key).toString());
            }
        }

        @Override
        protected JSONObject getNewJson() {
            JSONObject json;
            if (mRunMetrics != null) {
                json = new JSONObject(mRunMetrics);
            } else {
                json = new JSONObject();
            }
            try {
                json.put(END_TIME, mEndTime);
            } catch (JSONException e) {
                CLog.e(e);
            }
            return json;
        }
    }

    /** Helper for testLog information. */
    public static class TestLogEventInfo {
        public String mDataName = null;
        public LogDataType mLogType = null;
        public File mDataFile = null;

        public TestLogEventInfo(String dataName, LogDataType dataType, File dataFile) {
            mDataName = dataName;
            mLogType = dataType;
            mDataFile = dataFile;
        }

        public TestLogEventInfo(JSONObject jsonObject) throws JSONException {
            mDataName = jsonObject.getString(DATA_NAME_KEY);
            jsonObject.remove(DATA_NAME_KEY);
            mLogType = LogDataType.valueOf(jsonObject.getString(DATA_TYPE_KEY));
            jsonObject.remove(DATA_TYPE_KEY);
            mDataFile = new File(jsonObject.getString(DATA_FILE_KEY));
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                tags = new JSONObject();
                if (mDataName != null) {
                    tags.put(DATA_NAME_KEY, mDataName);
                }
                if (mLogType != null) {
                    tags.put(DATA_TYPE_KEY, mLogType.toString());
                }
                if (mDataFile != null) {
                    tags.put(DATA_FILE_KEY, mDataFile.getAbsolutePath());
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /** Helper for logAssociation information. */
    public static class LogAssociationEventInfo {
        public String mDataName = null;
        public LogFile mLoggedFile = null;

        public LogAssociationEventInfo(String dataName, LogFile loggedFile) {
            mDataName = dataName;
            mLoggedFile = loggedFile;
        }

        public LogAssociationEventInfo(JSONObject jsonObject) throws JSONException {
            mDataName = jsonObject.getString(DATA_NAME_KEY);
            jsonObject.remove(DATA_NAME_KEY);
            String file = jsonObject.getString(LOGGED_FILE_KEY);
            try {
                mLoggedFile = (LogFile) SerializationUtil.deserialize(new File(file), true);
            } catch (IOException e) {
                throw new JSONException(e.getMessage());
            } finally {
                FileUtil.deleteFile(new File(file));
            }
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                tags = new JSONObject();
                if (mDataName != null) {
                    tags.put(DATA_NAME_KEY, mDataName);
                }
                if (mLoggedFile != null) {
                    File serializedLoggedFile = SerializationUtil.serialize(mLoggedFile);
                    tags.put(LOGGED_FILE_KEY, serializedLoggedFile.getAbsolutePath());
                }
            } catch (JSONException | IOException e) {
                CLog.e(e);
                throw new RuntimeException(e);
            }
            return tags.toString();
        }
    }

    /** Helper for invocation started information. */
    public static class InvocationStartedEventInfo {
        public String mTestTag = null;
        public Long mStartTime = null;

        public InvocationStartedEventInfo(String testTag, Long startTime) {
            mTestTag = testTag;
            mStartTime = startTime;
        }

        public InvocationStartedEventInfo(JSONObject jsonObject) throws JSONException {
            mTestTag = jsonObject.getString(TEST_TAG_KEY);
            if (jsonObject.has(START_TIME)) {
                mStartTime = jsonObject.getLong(START_TIME);
            }
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                tags = new JSONObject();
                if (mTestTag != null) {
                    tags.put(TEST_TAG_KEY, mTestTag);
                }
                if (mStartTime != null) {
                    tags.put(START_TIME, mStartTime);
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
            return tags.toString();
        }
    }

    /** Helper for test module started information. */
    public static class TestModuleStartedEventInfo {
        public IInvocationContext mModuleContext;

        public TestModuleStartedEventInfo(IInvocationContext moduleContext) {
            mModuleContext = moduleContext;
        }

        public TestModuleStartedEventInfo(JSONObject jsonObject) throws JSONException {
            String file = jsonObject.getString(MODULE_CONTEXT_KEY);
            try {
                mModuleContext =
                        (IInvocationContext) SerializationUtil.deserialize(new File(file), true);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public String toString() {
            JSONObject tags = null;
            try {
                tags = new JSONObject();
                File serializedContext = SerializationUtil.serialize(mModuleContext);
                tags.put(MODULE_CONTEXT_KEY, serializedContext.getAbsolutePath());
                // For easier debugging on the events for modules, add the module name
                String moduleName =
                        mModuleContext
                                .getAttributes()
                                .getUniqueMap()
                                .get(ModuleDefinition.MODULE_NAME);
                if (moduleName != null) {
                    tags.put(MODULE_NAME, moduleName);
                }
            } catch (IOException | JSONException e) {
                CLog.e(e);
                throw new RuntimeException(e);
            }
            return tags.toString();
        }
    }
}
