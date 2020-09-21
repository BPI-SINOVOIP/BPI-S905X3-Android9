/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

import com.google.clearsilver.jsilver.data.Data;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;

public class AnnotationInstanceInfo implements Resolvable {
  private ClassInfo mType;
  private String mAnnotationName; // for debugging purposes TODO - remove
  private ArrayList<AnnotationValueInfo> mElementValues;
  private ArrayList<Resolution> mResolutions;

  public AnnotationInstanceInfo() {
      mType = null;
      mElementValues = new ArrayList<AnnotationValueInfo>();
    }

  public AnnotationInstanceInfo(ClassInfo type, AnnotationValueInfo[] elementValues) {
    mType = type;
    mElementValues = new ArrayList<AnnotationValueInfo>(Arrays.asList(elementValues));
  }

  ClassInfo type() {
    return mType;
  }

  public void setClass(ClassInfo cl) {
      mType = cl;
  }

  public void setSimpleAnnotationName(String name) {
      mAnnotationName = name;
  }

  ArrayList<AnnotationValueInfo> elementValues() {
    return mElementValues;
  }

  public void addElementValue(AnnotationValueInfo info) {
      mElementValues.add(info);
  }

  @Override
  public String toString() {
    StringBuilder str = new StringBuilder();
    str.append("@");
    if (mType == null) {
        str.append(mAnnotationName);
    } else {
        str.append(mType.qualifiedName());
    }
    str.append("(");

    for (AnnotationValueInfo value : mElementValues) {
      if (value.element() != null) {
          str.append(value.element().name());
          str.append("=");
      }

      str.append(value.valueString());
      if (value != mElementValues.get(mElementValues.size()-1)) {
        str.append(",");
      }
    }
    str.append(")");
    return str.toString();
  }

  public void addResolution(Resolution resolution) {
      if (mResolutions == null) {
          mResolutions = new ArrayList<Resolution>();
      }

      mResolutions.add(resolution);
  }

  public void printResolutions() {
      System.out.println("Resolutions for Annotation:");
      for (Resolution r : mResolutions) {
          System.out.println(r);
      }
  }

  public boolean resolveResolutions() {
      ArrayList<Resolution> resolutions = mResolutions;
      mResolutions = new ArrayList<Resolution>();

      boolean allResolved = true;
      for (Resolution resolution : resolutions) {
          StringBuilder qualifiedClassName = new StringBuilder();
          InfoBuilder.resolveQualifiedName(resolution.getValue(), qualifiedClassName,
                  resolution.getInfoBuilder());

          // if we still couldn't resolve it, save it for the next pass
          if ("".equals(qualifiedClassName.toString())) {
              mResolutions.add(resolution);
              allResolved = false;
          } else if ("annotationTypeName".equals(resolution.getVariable())) {
              setClass(InfoBuilder.Caches.obtainClass(qualifiedClassName.toString()));
          }
      }

      return allResolved;
  }

  /**
   * Convert the specified list of {@code AnnotationInstanceInfo} into an HDF-formatted list, and
   * add the HDF list into the specified {@code Data}.
   */
  public static void makeLinkListHDF(Data data, String base, AnnotationInstanceInfo[] annotations) {
    if (annotations == null) return;

    final int N = annotations.length;
    for (int i = 0; i < N; i++) {
      AnnotationInstanceInfo aii = annotations[i];
      final String aiiBase = base + "." + i;

      // Serialize data about the annotation element values
      for (int elemIdx = 0; elemIdx < aii.elementValues().size(); ++elemIdx) {
        final String elemBase = aiiBase + ".elementValues." + elemIdx;
        final AnnotationValueInfo value = aii.elementValues().get(elemIdx);
        data.setValue(elemBase + ".name", value.element().name());
        data.setValue(elemBase + ".value", value.valueString());
      }

      aii.type().makeShortDescrHDF(data, aiiBase);
    }
  }

  /**
   * Get a new list containing the set of annotations that are shared between
   * the input annotations collection and the set of allowed annotations.
   */
  public static ArrayList<AnnotationInstanceInfo> getAnnotationsIntersection(
          Collection<String> allowedAnnotations,
          Collection<? extends AnnotationInstanceInfo> allAnnotations) {
    ArrayList<AnnotationInstanceInfo> list = new ArrayList<AnnotationInstanceInfo>();
    java.util.Objects.requireNonNull(allowedAnnotations);
    if (allAnnotations != null) {
      for (AnnotationInstanceInfo info : allAnnotations) {
        if (allowedAnnotations.contains(info.type().qualifiedName())) {
          list.add(info);
        }
      }
    }
    return list;
  }

  /**
   * Get a new list containing the set of annotations that are shared between
   * the input annotations collection and the names of annotations passed in
   * the showAnnotations parameter
   */
  public static ArrayList<AnnotationInstanceInfo> getShowAnnotationsIntersection(
          Collection<? extends AnnotationInstanceInfo> annotations) {
    return getAnnotationsIntersection(Doclava.showAnnotations, annotations);
  }

  /**
   * Get a new list containing the set of annotations that are shared between
   * the input annotations collection and the names of annotations passed in
   * the hideAnnotations parameter
   */
  public static ArrayList<AnnotationInstanceInfo> getHideAnnotationsIntersection(
          Collection<? extends AnnotationInstanceInfo> annotations) {
    return getAnnotationsIntersection(Doclava.hideAnnotations, annotations);
  }
}
