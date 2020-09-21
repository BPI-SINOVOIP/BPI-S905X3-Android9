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

package util.build;

import com.android.tools.r8.CompilationMode;
import com.android.tools.r8.D8;
import com.android.tools.r8.D8Command;
import com.android.tools.r8.OutputMode;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.BasicFileAttributes;

public class D8BuildStep extends BuildStep {

  private final boolean deleteInputFileAfterBuild;
  private final D8Command.Builder builder;

  D8BuildStep(BuildFile inputFile, BuildFile outputFile, boolean deleteInputFileAfterBuild) {
    super(inputFile, outputFile);
    this.deleteInputFileAfterBuild = deleteInputFileAfterBuild;
    this.builder =
        D8Command.builder()
            .setMode(CompilationMode.DEBUG)
            .setMinApiLevel(1000)
            .setEnableDesugaring(false);
  }

  @Override
  boolean build() {

    if (super.build()) {
      try {
        builder.setOutput(Paths.get(outputFile.fileName.getAbsolutePath()), OutputMode.DexIndexed);
        Files.find(
                Paths.get(inputFile.fileName.getAbsolutePath()),
                1000,
                D8BuildStep::isJarOrClassFile)
            .forEach(
                p -> {
                  try {
                    builder.addProgramFiles(p);
                  } catch (Throwable e) {
                    e.printStackTrace();
                  }
                });
        D8.run(builder.build());
      } catch (Throwable e) {
        e.printStackTrace();
        return false;
      }
      if (deleteInputFileAfterBuild) {
        inputFile.fileName.delete();
      }
      return true;
    }
    return false;
  }

  @Override
  public int hashCode() {
    return inputFile.hashCode() ^ outputFile.hashCode();
  }

  @Override
  public boolean equals(Object obj) {
    if (super.equals(obj)) {
      D8BuildStep other = (D8BuildStep) obj;

      return inputFile.equals(other.inputFile) && outputFile.equals(other.outputFile);
    }
    return false;
  }

  private static boolean isJarOrClassFile(Path file, BasicFileAttributes attrs) {
    if (!attrs.isRegularFile()) {
      return false;
    }
    String name = file.getFileName().toString().toLowerCase();
    return name.endsWith(".jar") || name.endsWith(".class");
  }
}
