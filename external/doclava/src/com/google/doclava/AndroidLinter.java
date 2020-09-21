/*
 * Copyright (C) 2017 Google Inc.
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

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class AndroidLinter implements Linter {
  @Override
  public void lintField(FieldInfo field) {
    if (!shouldLint(field.containingClass())) return;
    lintCommon(field.position(), field.comment().tags());

    for (TagInfo tag : field.comment().tags()) {
      String text = tag.text();

      // Intent rules don't apply to support library
      if (field.containingClass().qualifiedName().startsWith("android.support.")) continue;

      if (field.name().contains("ACTION")) {
        boolean hasBehavior = false;
        boolean hasSdkConstant = false;
        for (AnnotationInstanceInfo a : field.annotations()) {
          hasBehavior |= a.type().qualifiedNameMatches("android",
              "annotation.BroadcastBehavior");
          hasSdkConstant |= a.type().qualifiedNameMatches("android",
              "annotation.SdkConstant");
        }

        if (text.contains("Broadcast Action:")
            || (text.contains("protected intent") && text.contains("system"))) {
          if (!hasBehavior) {
            Errors.error(Errors.BROADCAST_BEHAVIOR, field,
                "Field '" + field.name() + "' is missing @BroadcastBehavior");
          }
          if (!hasSdkConstant) {
            Errors.error(Errors.SDK_CONSTANT, field, "Field '" + field.name()
                + "' is missing @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)");
          }
        }

        if (text.contains("Activity Action:")) {
          if (!hasSdkConstant) {
            Errors.error(Errors.SDK_CONSTANT, field, "Field '" + field.name()
                + "' is missing @SdkConstant(SdkConstantType.ACTIVITY_INTENT_ACTION)");
          }
        }
      }
    }
  }

  @Override
  public void lintMethod(MethodInfo method) {
    if (!shouldLint(method.containingClass())) return;
    lintCommon(method.position(), method.comment().tags());
    lintCommon(method.position(), method.returnTags().tags());

    for (TagInfo tag : method.comment().tags()) {
      String text = tag.text();

      boolean hasAnnotation = false;
      for (AnnotationInstanceInfo a : method.annotations()) {
        if (a.type().qualifiedNameMatches("android", "annotation.RequiresPermission")) {
          hasAnnotation = true;
          ArrayList<AnnotationValueInfo> values = new ArrayList<>();
          for (AnnotationValueInfo val : a.elementValues()) {
            switch (val.element().name()) {
              case "value":
                values.add(val);
                break;
              case "allOf":
                values = (ArrayList<AnnotationValueInfo>) val.value();
                break;
              case "anyOf":
                values = (ArrayList<AnnotationValueInfo>) val.value();
                break;
            }
          }
          if (values.isEmpty()) continue;

          for (AnnotationValueInfo value : values) {
            String perm = String.valueOf(value.value());
            if (perm.indexOf('.') >= 0) perm = perm.substring(perm.lastIndexOf('.') + 1);
            if (text.contains(perm)) {
              Errors.error(Errors.REQUIRES_PERMISSION, method, "Method '" + method.name()
                  + "' documentation mentions permissions already declared by @RequiresPermission");
            }
          }
        }
      }
      if (text.contains("android.Manifest.permission") || text.contains("android.permission.")) {
        if (!hasAnnotation) {
          Errors.error(Errors.REQUIRES_PERMISSION, method, "Method '" + method.name()
              + "' documentation mentions permissions without declaring @RequiresPermission");
        }
      }
    }

    lintVariable(method.position(), "Return value of '" + method.name() + "'", method.returnType(),
        method.annotations(), method.returnTags().tags());
  }

  @Override
  public void lintParameter(MethodInfo method, ParameterInfo param, SourcePositionInfo position,
      TagInfo tag) {
    if (!shouldLint(method.containingClass())) return;
    lintCommon(position, tag);

    lintVariable(position, "Parameter '" + param.name() + "' of '" + method.name() + "'",
        param.type(), param.annotations(), tag);
  }

  private static void lintVariable(SourcePositionInfo pos, String ident, TypeInfo type,
      List<AnnotationInstanceInfo> annotations, TagInfo... tags) {
    if (type == null) return;
    for (TagInfo tag : tags) {
      String text = tag.text();

      if (type.simpleTypeName().equals("int")
          && Pattern.compile("[A-Z]{3,}_([A-Z]{3,}|\\*)").matcher(text).find()) {
        boolean hasAnnotation = false;
        for (AnnotationInstanceInfo a : annotations) {
          for (AnnotationInstanceInfo b : a.type().annotations()) {
            hasAnnotation |= b.type().qualifiedNameMatches("android", "annotation.IntDef");
          }
        }
        if (!hasAnnotation) {
          Errors.error(Errors.INT_DEF, pos,
              ident + " documentation mentions constants without declaring an @IntDef");
        }
      }

      if (Pattern.compile("\\bnull\\b").matcher(text).find()) {
        boolean hasAnnotation = false;
        for (AnnotationInstanceInfo a : annotations) {
          hasAnnotation |= a.type().qualifiedNameMatches("android", "annotation.NonNull");
          hasAnnotation |= a.type().qualifiedNameMatches("android", "annotation.Nullable");
        }
        if (!hasAnnotation) {
          Errors.error(Errors.NULLABLE, pos,
              ident + " documentation mentions 'null' without declaring @NonNull or @Nullable");
        }
      }
    }
  }

  private static void lintCommon(SourcePositionInfo pos, TagInfo... tags) {
    for (TagInfo tag : tags) {
      String text = tag.text();
      if (text.contains("TODO:") || text.contains("TODO(")) {
        Errors.error(Errors.TODO, pos, "Documentation mentions 'TODO'");
      }
    }
  }

  private static boolean shouldLint(ClassInfo clazz) {
    return clazz.qualifiedName().startsWith("android.")
        && !clazz.qualifiedName().startsWith("android.icu.");
  }
}
