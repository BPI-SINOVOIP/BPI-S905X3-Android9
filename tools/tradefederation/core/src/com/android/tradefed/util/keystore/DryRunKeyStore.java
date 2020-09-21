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
package com.android.tradefed.util.keystore;

/** A keystore for dry-run where any keystore value is always properly replaced and found. */
public class DryRunKeyStore implements IKeyStoreClient {

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public boolean containsKey(String key) {
        return true;
    }

    @Override
    public String fetchKey(String key) {
        return null;
    }

    /**
     * Special callback for the dry run keystore to create value of expected type.
     *
     * @param key the key for the option, will be ignored.
     * @param optionType the return type expected.
     */
    public String fetchKey(String key, String optionType) {
        switch (optionType) {
            case "boolean":
                return "true";
            case "long":
            case "int":
            case "integer":
            case "float":
            case "double":
                return "0";
            case "string":
                return "";
            default:
                return "";
        }
    }
}
