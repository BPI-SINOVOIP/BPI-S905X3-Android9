/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.cts.api;

/**
 * A method or constructor defined in a DEX or in an API file
 */
public final class DefinedMethod implements Comparable<DefinedMethod> {
    private final String signature;
    private final String definingClass;

    DefinedMethod(String signature, String definingClass) {
        this.signature = signature;
        this.definingClass = definingClass;
    }

    /**
     * Returns name(arg1,arg2,...)type. For constructor, name is <init> and type is empty.
     */
    String getSignature() {
        return signature;
    }

    /**
     * Returns class.name(arg1,arg2,...)type. For constructor, name is <init> and type is empty.
     */
    public String getFullSignature() {
        return getDefiningClass() + "." + getSignature();
    }

    String getDefiningClass() {
        return definingClass;
    }

    @Override
    public int compareTo(DefinedMethod o) {
        if (o != null) {
            return getFullSignature().compareTo(o.getFullSignature());
        }
        return 0;
    }

    @Override
    public int hashCode() {
        return getFullSignature().hashCode();
    }

    /**
     * Methods are fully identified by their full signature. Two methods with same signature are
     * considered to be the same.
     */
    @Override
    public boolean equals(Object o) {
        if (o != null && o instanceof DefinedMethod) {
            return getFullSignature().equals(((DefinedMethod) o).getFullSignature());
        }
        return false;
    }
}
