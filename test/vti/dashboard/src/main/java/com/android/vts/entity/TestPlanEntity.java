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

import com.google.appengine.api.datastore.Entity;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing test plan metadata. */
public class TestPlanEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(TestPlanEntity.class.getName());

    public static final String KIND = "TestPlan";

    // Property keys
    public static final String TEST_PLAN_NAME = "testPlanName";

    public final String testPlanName;

    /**
     * Create a TestPlanEntity object.
     *
     * @param testPlanName The name of the test plan.
     */
    public TestPlanEntity(String testPlanName) {
        this.testPlanName = testPlanName;
    }

    @Override
    public Entity toEntity() {
        Entity planEntity = new Entity(KIND, this.testPlanName);
        return planEntity;
    }

    /**
     * Convert an Entity object to a TestEntity.
     *
     * @param e The entity to process.
     * @return TestEntity object with the properties from e processed, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestPlanEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || e.getKey().getName() == null
                || !e.hasProperty(TEST_PLAN_NAME)) {
            logger.log(Level.WARNING, "Missing test plan attributes in entity: " + e.toString());
            return null;
        }
        String testPlanName = e.getKey().getName();
        return new TestPlanEntity(testPlanName);
    }
}
