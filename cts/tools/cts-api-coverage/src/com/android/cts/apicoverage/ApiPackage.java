/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.cts.apicoverage;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

/** Representation of a package in the API containing classes. */
class ApiPackage implements HasCoverage {

    private final String mName;

    private final Map<String, ApiClass> mApiClassMap = new HashMap<String, ApiClass>();

    ApiPackage(String name) {
        mName = name;
    }

    @Override
    public String getName() {
        return mName;
    }

    public void addClass(ApiClass apiClass) {
        mApiClassMap.put(apiClass.getName(), apiClass);
    }

    public ApiClass getClass(String name) {
        return mApiClassMap.get(name);
    }

    public Collection<ApiClass> getClasses() {
        return Collections.unmodifiableCollection(mApiClassMap.values());
    }

    public int getNumCoveredMethods() {
        int covered = 0;
        for (ApiClass apiClass : mApiClassMap.values()) {
            covered += apiClass.getNumCoveredMethods();
        }
        return covered;
    }

    public int getTotalMethods() {
        int total = 0;
        for (ApiClass apiClass : mApiClassMap.values()) {
            total += apiClass.getTotalMethods();
        }
        return total;
    }

    @Override
    public float getCoveragePercentage() {
        return (float) getNumCoveredMethods() / getTotalMethods() * 100;
    }

    @Override
    public int getMemberSize() {
        return getTotalMethods();
    }

    /** Iterate through all classes and add superclass. */
    public void resolveSuperClasses(Map<String, ApiPackage> packageMap) {
        Iterator<Entry<String, ApiClass>> it = mApiClassMap.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry<String, ApiClass> entry = it.next();
            ApiClass apiClass = entry.getValue();
            if (apiClass.getSuperClassName() != null) {
                String superClassName = apiClass.getSuperClassName();
                // Split the fully qualified class name into package and class name.
                String packageName = superClassName.substring(0, superClassName.lastIndexOf('.'));
                String className = superClassName.substring(
                        superClassName.lastIndexOf('.') + 1, superClassName.length());
                if (packageMap.containsKey(packageName)) {
                    ApiPackage apiPackage = packageMap.get(packageName);
                    ApiClass superClass = apiPackage.getClass(className);
                    if (superClass != null) {
                        // Add the super class
                        apiClass.setSuperClass(superClass);
                    }
                }
            }
        }
    }
}
