/*
 * Copyright (C) 2008 The Android Open Source Project
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

package util.build;

import java.io.File;
import java.lang.Iterable;
import java.util.stream.Collectors;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import javax.tools.JavaCompiler;
import javax.tools.JavaFileObject;
import javax.tools.StandardJavaFileManager;
import javax.tools.StandardLocation;
import javax.tools.ToolProvider;

public class JavacBuildStep extends SourceBuildStep {

    private final String destPath;
    private final String classPath;
    private final Set<String> sourceFiles = new HashSet<String>();
    public JavacBuildStep(String destPath, String classPath) {
        super(new File(destPath));
        this.destPath = destPath;
        this.classPath = classPath;
    }

    @Override
    public void addSourceFile(String sourceFile)
    {
        sourceFiles.add(sourceFile);
    }

    @Override
    boolean build() {
        if (super.build())
        {
            if (sourceFiles.isEmpty())
            {
                return true;
            }

            File destFile = new File(destPath);
            if (!destFile.exists() && !destFile.mkdirs())
            {
                System.err.println("failed to create destination dir");
                return false;
            }

            Iterable<File> classPathFiles = Arrays.asList(classPath.split(":"))
                    .stream()
                    .map(File::new)
                    .collect(Collectors.toList());

            JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
            try (StandardJavaFileManager fileManager = compiler.getStandardFileManager(
                    null,     // diagnosticListener: we don't care about the details.
                    null,     // locale: use default locale.
                    null)) {  // charset: use platform default.
                fileManager.setLocation(StandardLocation.CLASS_OUTPUT, Arrays.asList(
                        new File(destPath)));
                fileManager.setLocation(StandardLocation.CLASS_PATH, classPathFiles);

                Iterable<? extends JavaFileObject> compilationUnits =
                        fileManager.getJavaFileObjectsFromStrings(sourceFiles);

                List<String> options = Arrays.asList("-source", "1.7", "-target", "1.7");

                return compiler.getTask(
                        null,  // out: write errors to System.err.
                        fileManager,
                        null,  // diagnosticListener: we don't care about the details.
                        options,
                        null,  // classes: classes for annotation processing = none.
                        compilationUnits).call();
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }
        return false;
    }

    @Override
    public boolean equals(Object obj) {
        if (super.equals(obj))
        {
            JavacBuildStep other = (JavacBuildStep) obj;
            return destPath.equals(other.destPath)
                && classPath.equals(other.classPath)
                && sourceFiles.equals(other.sourceFiles);
        }
        return false;
    }

    @Override
    public int hashCode() {
        return destPath.hashCode() ^ classPath.hashCode() ^ sourceFiles.hashCode();
    }
}
