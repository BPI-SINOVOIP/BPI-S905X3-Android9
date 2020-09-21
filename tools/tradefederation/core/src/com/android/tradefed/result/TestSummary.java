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
package com.android.tradefed.result;

import java.util.HashMap;
import java.util.Map;

/**
 * A class to represent a test summary.  Provides a single field for a summary, in addition to a
 * key-value store for more detailed summary.  Also provides a place for the summary source to be
 * identified.
 */
public class TestSummary {
    private TypedString mSummary = null;
    private Map<String, TypedString> mKvStore = null;
    private String mSource = null;

    public static enum Type {
        URI("uri"),
        TEXT("text");
        private final String mType;
        Type() {
            mType = "uri";
        }

        Type(String type) {
            mType = type;
        }

        String getType() {
            return mType;
        }
    }

    public static class TypedString {
        private Type mType;
        private String mString;
        public TypedString(String string) {
            this(string, Type.URI);
        }
        public TypedString(String string, Type type) {
            mType = type;
            mString = string;
        }
        public Type getType() {
            return mType;
        }
        public String getString() {
            return mString;
        }

        @Override
        public String toString() {
            return String.format("%s: %s", mType.toString(), mString);
        }
    }

    /**
     * A convenience constructor that takes the string representation of a URI
     *
     * @param summaryUri A {@link String} representing a URI
     */
    public TestSummary(String summaryUri) {
        this(new TypedString(summaryUri));
    }

    public TestSummary(TypedString summary) {
        setSummary(summary);
        mKvStore = new HashMap<String, TypedString>();
    }

    /**
     * Set the source of the summary, in case a consumer wants to implement a contract with a
     * specific producer
     *
     * @param source A {@link String} containing the fully-qualified Java class name of the source
     */
    public void setSource(String source) {
        mSource = source;
    }
    public void setSummary(TypedString summary) {
        mSummary = summary;
    }
    public void addKvEntry(String key, TypedString value) {
        mKvStore.put(key, value);
    }

    // trampolines
    public void setSummary(String summary) {
        setSummary(new TypedString(summary));
    }
    public void addKvEntry(String key, String value) {
        addKvEntry(key, new TypedString(value));
    }

    public String getSource() {
        return mSource;
    }
    public TypedString getSummary() {
        return mSummary;
    }
    public Map<String, TypedString> getKvEntries() {
        return mKvStore;
    }
}
