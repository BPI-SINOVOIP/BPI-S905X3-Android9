/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.entity;

import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.proto.VtsReportMessage.VtsProfilingType;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing a profiling point. */
public class ProfilingPointEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(ProfilingPointEntity.class.getName());
    protected static final String DELIMITER = "#";

    public static final String KIND = "ProfilingPoint";

    // Property keys
    public static final String TEST_NAME = "testName";
    public static final String PROFILING_POINT_NAME = "profilingPointName";
    public static final String TYPE = "type";
    public static final String REGRESSION_MODE = "regressionMode";
    public static final String X_LABEL = "xLabel";
    public static final String Y_LABEL = "yLabel";

    public final Key key;

    public final String testName;
    public final String profilingPointName;
    public final VtsProfilingType type;
    public final VtsProfilingRegressionMode regressionMode;
    public final String xLabel;
    public final String yLabel;

    /**
     * Create a ProfilingPointEntity object.
     *
     * @param testName The name of test containing the profiling point.
     * @param profilingPointName The name of the profiling point.
     * @param type The (number) type of the profiling point data.
     * @param regressionMode The (number) mode to use for detecting regression.
     * @param xLabel The x axis label.
     * @param yLabel The y axis label.
     */
    public ProfilingPointEntity(
            String testName,
            String profilingPointName,
            int type,
            int regressionMode,
            String xLabel,
            String yLabel) {
        this.key = createKey(testName, profilingPointName);
        this.testName = testName;
        this.profilingPointName = profilingPointName;
        this.type = VtsProfilingType.valueOf(type);
        this.regressionMode = VtsProfilingRegressionMode.valueOf(regressionMode);
        this.xLabel = xLabel;
        this.yLabel = yLabel;
    }

    /**
     * Create a key for a ProfilingPointEntity.
     *
     * @param testName The name of test containing the profiling point.
     * @param profilingPointName The name of the profiling point.
     * @return a Key object for the ProfilingEntity in the database.
     */
    public static Key createKey(String testName, String profilingPointName) {
        return KeyFactory.createKey(KIND, testName + DELIMITER + profilingPointName);
    }

    @Override
    public Entity toEntity() {
        Entity profilingPoint = new Entity(key);
        profilingPoint.setIndexedProperty(TEST_NAME, this.testName);
        profilingPoint.setIndexedProperty(PROFILING_POINT_NAME, this.profilingPointName);
        profilingPoint.setUnindexedProperty(TYPE, this.type.getNumber());
        profilingPoint.setUnindexedProperty(REGRESSION_MODE, this.regressionMode.getNumber());
        profilingPoint.setUnindexedProperty(X_LABEL, this.xLabel);
        profilingPoint.setUnindexedProperty(Y_LABEL, this.yLabel);

        return profilingPoint;
    }

    /**
     * Convert an Entity object to a ProfilingPointEntity.
     *
     * @param e The entity to process.
     * @return ProfilingPointEntity object with the properties from e, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static ProfilingPointEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND)
                || e.getKey().getName() == null
                || !e.hasProperty(TEST_NAME)
                || !e.hasProperty(PROFILING_POINT_NAME)
                || !e.hasProperty(TYPE)
                || !e.hasProperty(REGRESSION_MODE)
                || !e.hasProperty(X_LABEL)
                || !e.hasProperty(Y_LABEL)) {
            logger.log(
                    Level.WARNING, "Missing profiling point attributes in entity: " + e.toString());
            return null;
        }
        try {
            String testName = (String) e.getProperty(TEST_NAME);
            String profilingPointName = (String) e.getProperty(PROFILING_POINT_NAME);
            int type = (int) (long) e.getProperty(TYPE);
            int regressionMode = (int) (long) e.getProperty(REGRESSION_MODE);
            String xLabel = (String) e.getProperty(X_LABEL);
            String yLabel = (String) e.getProperty(Y_LABEL);

            return new ProfilingPointEntity(
                    testName, profilingPointName, type, regressionMode, xLabel, yLabel);
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing profiling point entity.", exception);
        }
        return null;
    }
}
