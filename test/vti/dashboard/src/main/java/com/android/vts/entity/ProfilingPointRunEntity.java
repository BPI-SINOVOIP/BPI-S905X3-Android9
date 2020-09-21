/*
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
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

import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.proto.VtsReportMessage.VtsProfilingType;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.protobuf.ByteString;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing a profiling point execution. */
public class ProfilingPointRunEntity implements DashboardEntity {
    protected static final Logger logger =
            Logger.getLogger(ProfilingPointRunEntity.class.getName());

    public static final String KIND = "ProfilingPointRun";

    // Property keys
    public static final String TYPE = "type";
    public static final String REGRESSION_MODE = "regressionMode";
    public static final String LABELS = "labels";
    public static final String VALUES = "values";
    public static final String X_LABEL = "xLabel";
    public static final String Y_LABEL = "yLabel";
    public static final String OPTIONS = "options";

    public final Key key;

    public final String name;
    public final VtsProfilingType type;
    public final VtsProfilingRegressionMode regressionMode;
    public final List<String> labels;
    public final List<Long> values;
    public final String xLabel;
    public final String yLabel;
    public final List<String> options;

    /**
     * Create a ProfilingPointRunEntity object.
     *
     * @param parentKey The Key object for the parent TestRunEntity in the database.
     * @param name The name of the profiling point.
     * @param type The (number) type of the profiling point data.
     * @param regressionMode The (number) mode to use for detecting regression.
     * @param labels List of data labels, or null if the data is unlabeled.
     * @param values List of data values.
     * @param xLabel The x axis label.
     * @param yLabel The y axis label.
     * @param options The list of key=value options for the profiling point run.
     */
    public ProfilingPointRunEntity(
            Key parentKey,
            String name,
            int type,
            int regressionMode,
            List<String> labels,
            List<Long> values,
            String xLabel,
            String yLabel,
            List<String> options) {
        this.key = KeyFactory.createKey(parentKey, KIND, name);
        this.name = name;
        this.type = VtsProfilingType.valueOf(type);
        this.regressionMode = VtsProfilingRegressionMode.valueOf(regressionMode);
        this.labels = labels == null ? null : new ArrayList<>(labels);
        this.values = new ArrayList<>(values);
        this.xLabel = xLabel;
        this.yLabel = yLabel;
        this.options = options;
    }

    @Override
    public Entity toEntity() {
        Entity profilingRun = new Entity(this.key);
        profilingRun.setUnindexedProperty(TYPE, this.type.getNumber());
        profilingRun.setUnindexedProperty(REGRESSION_MODE, this.regressionMode.getNumber());
        if (this.labels != null) {
            profilingRun.setUnindexedProperty(LABELS, this.labels);
        }
        profilingRun.setUnindexedProperty(VALUES, this.values);
        profilingRun.setUnindexedProperty(X_LABEL, this.xLabel);
        profilingRun.setUnindexedProperty(Y_LABEL, this.yLabel);
        if (this.options != null) {
            profilingRun.setUnindexedProperty(OPTIONS, this.options);
        }

        return profilingRun;
    }

    /**
     * Convert an Entity object to a ProflilingPointRunEntity.
     *
     * @param e The entity to process.
     * @return ProfilingPointRunEntity object with the properties from e, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static ProfilingPointRunEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND)
                || e.getKey().getName() == null
                || !e.hasProperty(TYPE)
                || !e.hasProperty(REGRESSION_MODE)
                || !e.hasProperty(VALUES)
                || !e.hasProperty(X_LABEL)
                || !e.hasProperty(Y_LABEL)) {
            logger.log(
                    Level.WARNING, "Missing profiling point attributes in entity: " + e.toString());
            return null;
        }
        try {
            Key parentKey = e.getParent();
            String name = e.getKey().getName();
            int type = (int) (long) e.getProperty(TYPE);
            int regressionMode = (int) (long) e.getProperty(REGRESSION_MODE);
            List<Long> values = (List<Long>) e.getProperty(VALUES);
            String xLabel = (String) e.getProperty(X_LABEL);
            String yLabel = (String) e.getProperty(Y_LABEL);
            List<String> labels = null;
            if (e.hasProperty(LABELS)) {
                labels = (List<String>) e.getProperty(LABELS);
            }
            List<String> options = null;
            if (e.hasProperty(OPTIONS)) {
                options = (List<String>) e.getProperty(OPTIONS);
            }
            return new ProfilingPointRunEntity(
                    parentKey, name, type, regressionMode, labels, values, xLabel, yLabel, options);
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing profiling point run entity.", exception);
        }
        return null;
    }

    /**
     * Convert a coverage report to a CoverageEntity.
     *
     * @param parentKey The ancestor key for the coverage entity.
     * @param profilingReport The profiling report containing profiling data.
     * @return The ProfilingPointRunEntity for the profiling report message, or null if incompatible
     */
    public static ProfilingPointRunEntity fromProfilingReport(
            Key parentKey, ProfilingReportMessage profilingReport) {
        if (!profilingReport.hasName()
                || !profilingReport.hasType()
                || profilingReport.getType() == VtsProfilingType.UNKNOWN_VTS_PROFILING_TYPE
                || !profilingReport.hasRegressionMode()
                || !profilingReport.hasXAxisLabel()
                || !profilingReport.hasYAxisLabel()) {
            return null; // invalid profiling report;
        }
        String name = profilingReport.getName().toStringUtf8();
        VtsProfilingType type = profilingReport.getType();
        VtsProfilingRegressionMode regressionMode = profilingReport.getRegressionMode();
        String xLabel = profilingReport.getXAxisLabel().toStringUtf8();
        String yLabel = profilingReport.getYAxisLabel().toStringUtf8();
        List<Long> values;
        List<String> labels = null;
        switch (type) {
            case VTS_PROFILING_TYPE_TIMESTAMP:
                if (!profilingReport.hasStartTimestamp()
                        || !profilingReport.hasEndTimestamp()
                        || profilingReport.getEndTimestamp()
                                < profilingReport.getStartTimestamp()) {
                    return null; // missing timestamp
                }
                long value =
                        profilingReport.getEndTimestamp() - profilingReport.getStartTimestamp();
                values = new ArrayList<>();
                values.add(value);
                break;
            case VTS_PROFILING_TYPE_LABELED_VECTOR:
                if (profilingReport.getValueCount() != profilingReport.getLabelCount()) {
                    return null; // jagged data
                }
                labels = new ArrayList<>();
                for (ByteString label : profilingReport.getLabelList()) {
                    labels.add(label.toStringUtf8());
                }
                values = profilingReport.getValueList();
                break;
            case VTS_PROFILING_TYPE_UNLABELED_VECTOR:
                values = profilingReport.getValueList();
                break;
            default: // should never happen
                return null;
        }
        List<String> options = null;
        if (profilingReport.getOptionsCount() > 0) {
            options = new ArrayList<>();
            for (ByteString optionBytes : profilingReport.getOptionsList()) {
                options.add(optionBytes.toStringUtf8());
            }
        }
        return new ProfilingPointRunEntity(
                parentKey,
                name,
                type.getNumber(),
                regressionMode.getNumber(),
                labels,
                values,
                xLabel,
                yLabel,
                options);
    }
}
