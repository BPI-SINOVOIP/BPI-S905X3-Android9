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
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing a branch. */
public class BranchEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(BranchEntity.class.getName());

    public static final String KIND = "Branch"; // The entity kind.

    public Key key; // The key for the entity in the database.

    /**
     * Create a BranchEntity object.
     *
     * @param branchName The name of the branch.
     */
    public BranchEntity(String branchName) {
        this.key = KeyFactory.createKey(KIND, branchName);
    }

    @Override
    public Entity toEntity() {
        return new Entity(this.key);
    }

    /**
     * Convert an Entity object to a BranchEntity.
     *
     * @param e The entity to process.
     * @return BranchEntity object with the properties from e, or null if incompatible.
     */
    public static BranchEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || e.getKey().getName() == null) {
            logger.log(Level.WARNING, "Missing branch attributes in entity: " + e.toString());
            return null;
        }
        String branchName = e.getKey().getName();
        return new BranchEntity(branchName);
    }
}
