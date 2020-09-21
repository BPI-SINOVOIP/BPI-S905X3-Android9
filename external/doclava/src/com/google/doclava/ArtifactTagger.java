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

package com.google.doclava;

import com.google.clearsilver.jsilver.data.Data;
import com.google.doclava.apicheck.ApiCheck;
import com.google.doclava.apicheck.ApiInfo;
import com.google.doclava.apicheck.ApiParseException;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Applies version information to the Doclava class model from apicheck XML files.
 * <p>
 * Sample usage:
 * <pre>
 *   ClassInfo[] classInfos = ...
 *
 *   ArtifactTagger artifactTagger = new ArtifactTagger()
 *   artifactTagger.addArtifact("frameworks/support/core-ui/api/current.xml",
 *       "com.android.support:support-core-ui:26.0.0")
 *   artifactTagger.addArtifact("frameworks/support/design/api/current.xml",
 *       "com.android.support:support-design:26.0.0")
 *   artifactTagger.tagAll(...);
 * </pre>
 */
public class ArtifactTagger {

  private final Map<String, String> xmlToArtifact = new LinkedHashMap<>();

  /**
   * Specifies the apicheck XML file associated with an artifact.
   * <p>
   * This method should only be called once per artifact.
   *
   * @param file an apicheck XML file
   * @param mavenSpec the Maven spec for the artifact to which the XML file belongs
   */
  public void addArtifact(String file, String mavenSpec) {
    xmlToArtifact.put(file, mavenSpec);
  }

  /**
   * Tags the specified docs with artifact information.
   *
   * @param classDocs the docs to tag
   */
  public void tagAll(Collection<ClassInfo> classDocs) {
    // Read through the XML files in order, applying their artifact information
    // to the Javadoc models.
    for (Map.Entry<String, String> artifactSpec : xmlToArtifact.entrySet()) {
      String xmlFile = artifactSpec.getKey();
      String artifactName = artifactSpec.getValue();

      ApiInfo specApi;
      try {
        specApi = new ApiCheck().parseApi(xmlFile);
      } catch (ApiParseException e) {
        StringWriter stackTraceWriter = new StringWriter();
        e.printStackTrace(new PrintWriter(stackTraceWriter));
        Errors.error(Errors.BROKEN_ARTIFACT_FILE, (SourcePositionInfo) null,
            "Failed to parse " + xmlFile + " for " + artifactName + " artifact data.\n"
                + stackTraceWriter.toString());
        continue;
      }

      applyArtifactsFromSpec(artifactName, specApi, classDocs);
    }

    if (!xmlToArtifact.isEmpty()) {
      warnForMissingArtifacts(classDocs);
    }
  }

  /**
   * Returns {@code true} if any artifact mappings are specified.
   */
  public boolean hasArtifacts() {
    return !xmlToArtifact.isEmpty();
  }

  /**
   * Writes an index of the artifact names to {@code data}.
   */
  public void writeArtifactNames(Data data) {
    int index = 1;
    for (String artifact : xmlToArtifact.values()) {
      data.setValue("artifact." + index + ".name", artifact);
      index++;
    }
  }

  /**
   * Applies artifact information to {@code classDocs} where not already present.
   *
   * @param mavenSpec the Maven spec for the artifact
   * @param specApi the spec for this artifact
   * @param classDocs the docs to update
   */
  private void applyArtifactsFromSpec(String mavenSpec, ApiInfo specApi,
      Collection<ClassInfo> classDocs) {
    for (ClassInfo classDoc : classDocs) {
      PackageInfo packageSpec = specApi.getPackages().get(classDoc.containingPackage().name());
      if (packageSpec != null) {
        ClassInfo classSpec = packageSpec.allClasses().get(classDoc.name());
        if (classSpec != null) {
          if (classDoc.getArtifact() == null) {
            classDoc.setArtifact(mavenSpec);
          } else {
            Errors.error(Errors.BROKEN_ARTIFACT_FILE, (SourcePositionInfo) null, "Class "
                + classDoc.name() + " belongs to multiple artifacts: " + classDoc.getArtifact()
                + " and " + mavenSpec);
          }
        }
      }
    }
  }

  /**
   * Warns if any symbols are missing artifact information. When configured properly, this will
   * yield zero warnings because {@code apicheck} guarantees that all symbols are present in the
   * most recent API.
   *
   * @param classDocs the docs to verify
   */
  private void warnForMissingArtifacts(Collection<ClassInfo> classDocs) {
    for (ClassInfo claz : classDocs) {
      if (checkLevelRecursive(claz) && claz.getArtifact() == null) {
        Errors.error(Errors.NO_ARTIFACT_DATA, claz.position(), "XML missing class "
            + claz.qualifiedName());
      }
    }
  }

  /**
   * Returns true if {@code claz} and all containing classes are documented. The result may be used
   * to filter out members that exist in the API data structure but aren't a part of the API.
   */
  private boolean checkLevelRecursive(ClassInfo claz) {
    for (ClassInfo c = claz; c != null; c = c.containingClass()) {
      if (!c.checkLevel()) {
        return false;
      }
    }
    return true;
  }
}
