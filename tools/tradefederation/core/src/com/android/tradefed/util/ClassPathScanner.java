/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.ddmlib.Log;

import java.io.File;
import java.io.IOException;
import java.util.Enumeration;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.jar.JarFile;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;

/**
 * Finds entries on classpath.
 *
 * <p>Adapted from vogar.target.ClassPathScanner</p>
 */
public class ClassPathScanner {

    private static final String LOG_TAG = "ClassPathScanner";
    private String[] mClassPath;

    /**
     * A filter for classpath entry paths
     * <p/>
     * Patterned after {@link java.io.FileFilter}
     */
    public static interface IClassPathFilter {
        /**
         * Tests whether or not the specified abstract pathname should be included in a class path
         * entry list.
         *
         * @param pathName the relative path of the class path entry
         */
        boolean accept(String pathName);

        /**
         * An optional converter for a class path entry path names.
         *
         * @param pathName the relative path of the class path entry, in format "foo/path/file.ext".
         * @return the pathName converted into context specific format
         */

        String transform(String pathName);
    }

    /**
     * A {@link IClassPathFilter} that filters and transforms java class names.
     */
    public static class ClassNameFilter implements IClassPathFilter {
        private static final String DOT_CLASS = ".class";

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean accept(String pathName) {
            return pathName.endsWith(DOT_CLASS);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public String transform(String pathName) {
            String className = pathName.substring(0, pathName.length() - DOT_CLASS.length());
            className = className.replace('/', '.');
            return className;
        }

    }

    /**
     * A {@link ClassNameFilter} that rejects inner classes
     */
    public static class ExternalClassNameFilter extends  ClassNameFilter {
        /**
         * {@inheritDoc}
         */
        @Override
        public boolean accept(String pathName) {
            return super.accept(pathName) && !pathName.contains("$");
        }
    }

    public ClassPathScanner() {
        mClassPath = getClassPath();
    }

    /**
     * Gets the names of all entries contained in given jar file, that match given filter
     * @throws IOException
     */
    public Set<String> getEntriesFromJar(File plainFile, IClassPathFilter filter)
            throws IOException {
        Set<String> entryNames = new LinkedHashSet<String>();
        JarFile jarFile = new JarFile(plainFile);
        for (Enumeration<? extends ZipEntry> e = jarFile.entries(); e.hasMoreElements(); ) {
            String entryName = e.nextElement().getName();
            if (filter.accept(entryName)) {
                entryNames.add(filter.transform(entryName));
            }
            entryName = null;
        }
        jarFile.close();
        return entryNames;
    }

    /**
     * Gets the names of all entries contained in given class path directory, that match given
     * filter
     * @throws IOException
     */
    public Set<String> getEntriesFromDir(File classPathDir, IClassPathFilter filter)
            throws IOException {
        Set<String> entryNames = new LinkedHashSet<String>();
        getEntriesFromDir(classPathDir, entryNames, new LinkedList<String>(), filter);
        return entryNames;
    }

    /**
     * Recursively adds the names of all entries contained in given class path directory,
     * that match given filter.
     *
     * @param dir the directory to scan
     * @param entries the {@link Set} of class path entry names to add to
     * @param rootPath the relative path of <var>dir</var> from class path element root
     * @param filter the {@link IClassPathFilter} to use
     * @throws IOException
     */
    private void getEntriesFromDir(File dir, Set<String> entries, List<String> rootPath,
            IClassPathFilter filter) throws IOException {
        File[] childFiles = dir.listFiles();
        if (childFiles == null) {
            Log.w(LOG_TAG, String.format("Directory %s in classPath is not readable, skipping",
                    dir.getAbsolutePath()));
            return;
        }
        for (File childFile : childFiles) {
            if (childFile.isDirectory()) {
                rootPath.add(childFile.getName() + "/");
                getEntriesFromDir(childFile, entries, rootPath, filter);
                // pop off the path element for this directory
                rootPath.remove(rootPath.size() - 1);
            } else if (childFile.isFile()) {
                // construct relative path of this file
                String classPathEntryName = constructPath(rootPath, childFile.getName());
                if (filter.accept(classPathEntryName)) {
                    entries.add(filter.transform(classPathEntryName));
                }
            } else {
                Log.d(LOG_TAG, String.format("file %s in classPath is not recognized, skipping",
                        dir.getAbsolutePath()));
            }
        }
    }

    /**
     * Construct a relative class path path for the given class path file
     *
     * @param rootPath the root path in {@link List} form
     * @param fileName the file name
     * @return the relative classpath path
     */
    private String constructPath(List<String> rootPath, String fileName) {
        StringBuilder pathBuilder = new StringBuilder();
        for (String element : rootPath) {
            pathBuilder.append(element);
        }
        pathBuilder.append(fileName);
        return pathBuilder.toString();
    }

    /**
     * Retrieves set of classpath entries that match given {@link IClassPathFilter}
     */
    public Set<String> getClassPathEntries(IClassPathFilter filter) {
        Set<String> entryNames = new LinkedHashSet<String>();
        for (String classPathElement : mClassPath) {
            File classPathFile = new File(classPathElement);
            try {
                if (classPathFile.isFile() && classPathElement.endsWith(".jar")) {
                    entryNames.addAll(getEntriesFromJar(classPathFile, filter));
                } else if (classPathFile.isDirectory()) {
                    entryNames.addAll(getEntriesFromDir(classPathFile, filter));
                } else {
                    Log.w(LOG_TAG, String.format(
                            "class path entry %s does not exist or is not recognized, skipping",
                            classPathElement));
                }
            } catch (IOException e) {
                Log.w(LOG_TAG, String.format("Failed to read class path entry %s. Reason: %s",
                        classPathElement, e.toString()));
            }
        }
        return entryNames;
    }

    /**
     * Gets the class path from the System Property "java.class.path" and splits
     * it up into the individual elements.
     */
    public static String[] getClassPath() {
        String classPath = System.getProperty("java.class.path");
        return classPath.split(Pattern.quote(File.pathSeparator));
    }
}
