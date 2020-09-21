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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

public class AndroidAuxSource implements AuxSource {
  private static final int TYPE_CLASS = 0;
  private static final int TYPE_FIELD = 1;
  private static final int TYPE_METHOD = 2;
  private static final int TYPE_PARAM = 3;
  private static final int TYPE_RETURN = 4;

  @Override
  public TagInfo[] classAuxTags(ClassInfo clazz) {
    if (hasSuppress(clazz.annotations())) return TagInfo.EMPTY_ARRAY;
    ArrayList<TagInfo> tags = new ArrayList<>();
    for (AnnotationInstanceInfo annotation : clazz.annotations()) {
      // Document system services
      if (annotation.type().qualifiedNameMatches("android", "annotation.SystemService")) {
        ArrayList<TagInfo> valueTags = new ArrayList<>();
        valueTags
            .add(new ParsedTagInfo("", "",
                "{@link android.content.Context#getSystemService(Class)"
                    + " Context.getSystemService(Class)}",
                null, SourcePositionInfo.UNKNOWN));
        valueTags.add(new ParsedTagInfo("", "",
            "{@code " + clazz.name() + ".class}", null,
            SourcePositionInfo.UNKNOWN));

        ClassInfo contextClass = annotation.type().findClass("android.content.Context");
        for (AnnotationValueInfo val : annotation.elementValues()) {
          switch (val.element().name()) {
            case "value":
              final String expected = String.valueOf(val.value());
              for (FieldInfo field : contextClass.fields()) {
                if (field.isHiddenOrRemoved()) continue;
                if (String.valueOf(field.constantValue()).equals(expected)) {
                  valueTags.add(new ParsedTagInfo("", "",
                      "{@link android.content.Context#getSystemService(String)"
                          + " Context.getSystemService(String)}",
                      null, SourcePositionInfo.UNKNOWN));
                  valueTags.add(new ParsedTagInfo("", "",
                      "{@link android.content.Context#" + field.name()
                          + " Context." + field.name() + "}",
                      null, SourcePositionInfo.UNKNOWN));
                }
              }
              break;
          }
        }

        Map<String, String> args = new HashMap<>();
        tags.add(new AuxTagInfo("@service", "@service", SourcePositionInfo.UNKNOWN, args,
            valueTags.toArray(TagInfo.getArray(valueTags.size()))));
      }
    }
    auxTags(TYPE_CLASS, clazz.annotations(), toString(clazz.inlineTags()), tags);
    return tags.toArray(TagInfo.getArray(tags.size()));
  }

  @Override
  public TagInfo[] fieldAuxTags(FieldInfo field) {
    if (hasSuppress(field)) return TagInfo.EMPTY_ARRAY;
    return auxTags(TYPE_FIELD, field.annotations(), toString(field.inlineTags()));
  }

  @Override
  public TagInfo[] methodAuxTags(MethodInfo method) {
    if (hasSuppress(method)) return TagInfo.EMPTY_ARRAY;
    return auxTags(TYPE_METHOD, method.annotations(), toString(method.inlineTags().tags()));
  }

  @Override
  public TagInfo[] paramAuxTags(MethodInfo method, ParameterInfo param, String comment) {
    if (hasSuppress(method)) return TagInfo.EMPTY_ARRAY;
    if (hasSuppress(param.annotations())) return TagInfo.EMPTY_ARRAY;
    return auxTags(TYPE_PARAM, param.annotations(), new String[] { comment });
  }

  @Override
  public TagInfo[] returnAuxTags(MethodInfo method) {
    if (hasSuppress(method)) return TagInfo.EMPTY_ARRAY;
    return auxTags(TYPE_RETURN, method.annotations(), toString(method.returnTags().tags()));
  }

  private static TagInfo[] auxTags(int type, List<AnnotationInstanceInfo> annotations,
      String[] comment) {
    ArrayList<TagInfo> tags = new ArrayList<>();
    auxTags(type, annotations, comment, tags);
    return tags.toArray(TagInfo.getArray(tags.size()));
  }

  private static void auxTags(int type, List<AnnotationInstanceInfo> annotations,
      String[] comment, ArrayList<TagInfo> tags) {
    for (AnnotationInstanceInfo annotation : annotations) {
      // Ignore null-related annotations when docs already mention
      if (annotation.type().qualifiedNameMatches("android", "annotation.NonNull")
          || annotation.type().qualifiedNameMatches("android", "annotation.Nullable")) {
        boolean mentionsNull = false;
        for (String c : comment) {
          mentionsNull |= Pattern.compile("\\bnull\\b").matcher(c).find();
        }
        if (mentionsNull) {
          continue;
        }
      }

      // Blindly include docs requested by annotations
      ParsedTagInfo[] docTags = ParsedTagInfo.EMPTY_ARRAY;
      switch (type) {
        case TYPE_METHOD:
        case TYPE_FIELD:
        case TYPE_CLASS:
          docTags = annotation.type().comment().memberDocTags();
          break;
        case TYPE_PARAM:
          docTags = annotation.type().comment().paramDocTags();
          break;
        case TYPE_RETURN:
          docTags = annotation.type().comment().returnDocTags();
          break;
      }
      for (ParsedTagInfo docTag : docTags) {
        tags.add(docTag);
      }

      // Document required permissions
      if ((type == TYPE_CLASS || type == TYPE_METHOD || type == TYPE_FIELD)
          && annotation.type().qualifiedNameMatches("android", "annotation.RequiresPermission")) {
        ArrayList<AnnotationValueInfo> values = new ArrayList<>();
        boolean any = false;
        for (AnnotationValueInfo val : annotation.elementValues()) {
          switch (val.element().name()) {
            case "value":
              values.add(val);
              break;
            case "allOf":
              values = (ArrayList<AnnotationValueInfo>) val.value();
              break;
            case "anyOf":
              any = true;
              values = (ArrayList<AnnotationValueInfo>) val.value();
              break;
          }
        }
        if (values.isEmpty()) continue;

        ClassInfo permClass = annotation.type().findClass("android.Manifest.permission");
        ArrayList<TagInfo> valueTags = new ArrayList<>();
        for (AnnotationValueInfo value : values) {
          final String expected = String.valueOf(value.value());
          for (FieldInfo field : permClass.fields()) {
            if (field.isHiddenOrRemoved()) continue;
            if (String.valueOf(field.constantValue()).equals(expected)) {
              valueTags.add(new ParsedTagInfo("", "",
                  "{@link " + permClass.qualifiedName() + "#" + field.name() + "}", null,
                  SourcePositionInfo.UNKNOWN));
            }
          }
        }

        Map<String, String> args = new HashMap<>();
        if (any) args.put("any", "true");
        tags.add(new AuxTagInfo("@permission", "@permission", SourcePositionInfo.UNKNOWN, args,
            valueTags.toArray(TagInfo.getArray(valueTags.size()))));
      }

      // Document required features
      if ((type == TYPE_CLASS || type == TYPE_METHOD || type == TYPE_FIELD)
          && annotation.type().qualifiedNameMatches("android", "annotation.RequiresFeature")) {
        AnnotationValueInfo value = null;
        for (AnnotationValueInfo val : annotation.elementValues()) {
          switch (val.element().name()) {
            case "value":
              value = val;
              break;
          }
        }
        if (value == null) continue;

        ClassInfo pmClass = annotation.type().findClass("android.content.pm.PackageManager");
        ArrayList<TagInfo> valueTags = new ArrayList<>();
        final String expected = String.valueOf(value.value());
        for (FieldInfo field : pmClass.fields()) {
          if (field.isHiddenOrRemoved()) continue;
          if (String.valueOf(field.constantValue()).equals(expected)) {
            valueTags.add(new ParsedTagInfo("", "",
                "{@link " + pmClass.qualifiedName() + "#" + field.name() + "}", null,
                SourcePositionInfo.UNKNOWN));
          }
        }

        valueTags.add(new ParsedTagInfo("", "",
            "{@link android.content.pm.PackageManager#hasSystemFeature(String)"
                + " PackageManager.hasSystemFeature(String)}",
            null, SourcePositionInfo.UNKNOWN));

        Map<String, String> args = new HashMap<>();
        tags.add(new AuxTagInfo("@feature", "@feature", SourcePositionInfo.UNKNOWN, args,
            valueTags.toArray(TagInfo.getArray(valueTags.size()))));
      }

      // The remaining annotations below always appear on return docs, and
      // should not be included in the method body
      if (type == TYPE_METHOD) continue;

      // Document value ranges
      if (annotation.type().qualifiedNameMatches("android", "annotation.IntRange")
          || annotation.type().qualifiedNameMatches("android", "annotation.FloatRange")) {
        String from = null;
        String to = null;
        for (AnnotationValueInfo val : annotation.elementValues()) {
          switch (val.element().name()) {
            case "from": from = String.valueOf(val.value()); break;
            case "to": to = String.valueOf(val.value()); break;
          }
        }
        if (from != null || to != null) {
          Map<String, String> args = new HashMap<>();
          if (from != null) args.put("from", from);
          if (to != null) args.put("to", to);
          tags.add(new AuxTagInfo("@range", "@range", SourcePositionInfo.UNKNOWN, args,
              TagInfo.EMPTY_ARRAY));
        }
      }

      // Document integer values
      for (AnnotationInstanceInfo inner : annotation.type().annotations()) {
        boolean intDef = inner.type().qualifiedNameMatches("android", "annotation.IntDef");
        boolean stringDef = inner.type().qualifiedNameMatches("android", "annotation.StringDef");
        if (intDef || stringDef) {
          ArrayList<AnnotationValueInfo> prefixes = null;
          ArrayList<AnnotationValueInfo> suffixes = null;
          ArrayList<AnnotationValueInfo> values = null;
          final String kind = intDef ? "@intDef" : "@stringDef";
          boolean flag = false;

          for (AnnotationValueInfo val : inner.elementValues()) {
            switch (val.element().name()) {
              case "prefix": prefixes = (ArrayList<AnnotationValueInfo>) val.value(); break;
              case "suffix": suffixes = (ArrayList<AnnotationValueInfo>) val.value(); break;
              case "value": values = (ArrayList<AnnotationValueInfo>) val.value(); break;
              case "flag": flag = Boolean.parseBoolean(String.valueOf(val.value())); break;
            }
          }

          // Sadly we can only generate docs when told about a prefix/suffix
          if (prefixes == null) prefixes = new ArrayList<>();
          if (suffixes == null) suffixes = new ArrayList<>();
          if (prefixes.isEmpty() && suffixes.isEmpty()) continue;

          final ClassInfo clazz = annotation.type().containingClass();
          final HashMap<String, FieldInfo> candidates = new HashMap<>();
          for (FieldInfo field : clazz.fields()) {
            if (field.isHiddenOrRemoved()) continue;
            for (AnnotationValueInfo prefix : prefixes) {
              if (field.name().startsWith(String.valueOf(prefix.value()))) {
                candidates.put(String.valueOf(field.constantValue()), field);
              }
            }
            for (AnnotationValueInfo suffix : suffixes) {
              if (field.name().endsWith(String.valueOf(suffix.value()))) {
                candidates.put(String.valueOf(field.constantValue()), field);
              }
            }
          }

          ArrayList<TagInfo> valueTags = new ArrayList<>();
          for (AnnotationValueInfo value : values) {
            final String expected = String.valueOf(value.value());
            final FieldInfo field = candidates.remove(expected);
            if (field != null) {
              valueTags.add(new ParsedTagInfo("", "",
                  "{@link " + clazz.qualifiedName() + "#" + field.name() + "}", null,
                  SourcePositionInfo.UNKNOWN));
            }
          }

          if (!valueTags.isEmpty()) {
            Map<String, String> args = new HashMap<>();
            if (flag) args.put("flag", "true");
            tags.add(new AuxTagInfo(kind, kind, SourcePositionInfo.UNKNOWN, args,
                valueTags.toArray(TagInfo.getArray(valueTags.size()))));
          }
        }
      }
    }
  }

  private static String[] toString(TagInfo[] tags) {
    final String[] res = new String[tags.length];
    for (int i = 0; i < res.length; i++) {
      res[i] = tags[i].text();
    }
    return res;
  }

  private static boolean hasSuppress(MemberInfo member) {
    return hasSuppress(member.annotations())
        || hasSuppress(member.containingClass().annotations());
  }

  private static boolean hasSuppress(List<AnnotationInstanceInfo> annotations) {
    for (AnnotationInstanceInfo annotation : annotations) {
      if (annotation.type().qualifiedNameMatches("android", "annotation.SuppressAutoDoc")) {
        return true;
      }
    }
    return false;
  }
}
