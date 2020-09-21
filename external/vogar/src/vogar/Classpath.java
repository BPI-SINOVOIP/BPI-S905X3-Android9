/*
 * Copyright (C) 2009 The Android Open Source Project
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

package vogar;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import vogar.util.Strings;

/**
 * A list of jar files and directories.
 */
public final class Classpath {

    private final List<File> elements = new ArrayList<File>();

    public static Classpath of(File... files) {
        return of(Arrays.asList(files));
    }

    public static Classpath of(Collection<File> files) {
        Classpath result = new Classpath();
        result.elements.addAll(files);
        return result;
    }

    public void addAll(File... elements) {
        addAll(Arrays.asList(elements));
    }

    public void addAll(Collection<File> elements) {
        this.elements.addAll(elements);
    }

    public void addAll(Classpath anotherClasspath) {
        this.elements.addAll(anotherClasspath.elements);
    }

    public Collection<File> getElements() {
        return elements;
    }

    public boolean isEmpty() {
        return elements.isEmpty();
    }

    public boolean contains(File file) {
        return elements.contains(file);
    }

    @Override public String toString() {
        return Strings.join(elements, ":");
    }
}
