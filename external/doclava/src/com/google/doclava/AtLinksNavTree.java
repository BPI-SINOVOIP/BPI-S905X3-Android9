/*
 * Copyright (C) 2016 Google Inc.
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

/**
* Class for writing a JSON dictionary of Android package/class/member info that can be used for
* dereferencing javadoc style {@link} tags.
*/

public class AtLinksNavTree {

  /**
  * Write a JSON dictionary of Android package/class/member info. The hierarchy will follow this
  * format: package name -> class name -> member name and signature -> parent package.class name
  * if the member was inherited (or an empty string if the member was not inherited).
  *
  * <p>For example:
  * <pre>{
  *   "android": {
  *     "Manifest": {
  *       "Manifest()": "",
  *       "clone()": "java.lang.Object",
  *       "equals(java.lang.Object)": "java.lang.Object",
  *       ...
  *     },
  *     ...
  *   },
  *   ...
  * }</pre>
  *
  * @param dir The directory path to prepend to the output path if the generated navtree is for
  *        one the a supplemental library references (such as the wearable support library)
  */
  public static void writeAtLinksNavTree(String dir) {
    StringBuilder buf = new StringBuilder();

    buf.append("{");
    addPackages(buf, Doclava.choosePackages());
    buf.append("\n}");

    Data data = Doclava.makeHDF();
    data.setValue("navtree", buf.toString());

    String output_path;
    if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS && (Doclava.libraryRoot != null)) {
      output_path = dir + Doclava.libraryRoot + "at_links_navtree.json";
    } else {
      output_path = "at_links_navtree.json";
    }

    ClearPage.write(data, "at_links_navtree.cs", output_path);
  }

  /**
  * Append the provided string builder with the navtree info for the provided list of packages.
  *
  * @param buf The string builder to append to.
  * @param packages A list of PackageInfo objects. Navtree info for each package will be appended
  *        to the provided string builder.
  */
  private static void addPackages(StringBuilder buf, PackageInfo[] packages) {
    boolean is_first_package = true;
    for (PackageInfo pkg : Doclava.choosePackages()) {
      if (!pkg.name().contains(".internal.")) {
        if (!is_first_package) {
          buf.append(",");
        }
        buf.append("\n  \"" + pkg.name() + "\": {");

        boolean is_first_class = true;
        is_first_class = addClasses(buf, pkg.annotations(), is_first_class);
        is_first_class = addClasses(buf, pkg.interfaces(), is_first_class);
        is_first_class = addClasses(buf, pkg.ordinaryClasses(), is_first_class);
        is_first_class = addClasses(buf, pkg.enums(), is_first_class);
        is_first_class = addClasses(buf, pkg.exceptions(), is_first_class);
        addClasses(buf, pkg.errors(), is_first_class);

        buf.append("\n  }");
        is_first_package = false;
      }
    }
  }

  /**
  * Append the provided string builder with the navtree info for the provided list of classes.
  *
  * @param buf The string builder to append to.
  * @param classes A list of ClassInfo objects. Navtree info for each class will be appended
  *        to the provided string builder.
  * @param is_first_class True if this is the first child class listed under the parent package.
  */
  private static boolean addClasses(StringBuilder buf, ClassInfo[] classes,
      boolean is_first_class) {
    for (ClassInfo cl : classes) {
      if (!is_first_class) {
        buf.append(",");
      }
      buf.append("\n    \"" + cl.name() + "\": {");

      boolean is_first_member = true;
      is_first_member = addFields(buf, cl.fields(), is_first_member, cl);
      is_first_member = addMethods(buf, cl.constructors(), is_first_member, cl);
      addMethods(buf, cl.methods(), is_first_member, cl);

      buf.append("\n    }");
      is_first_class = false;
    }
    return is_first_class;
  }

  /**
  * Append the provided string builder with the navtree info for the provided list of fields.
  *
  * @param buf The string builder to append to.
  * @param fields A list of FieldInfo objects. Navtree info for each field will be appended
  *        to the provided string builder.
  * @param is_first_member True if this is the first child member listed under the parent class.
  * @param cl The ClassInfo object for the parent class of this field.
  */
  private static boolean addFields(StringBuilder buf, ArrayList<FieldInfo> fields,
      boolean is_first_member, ClassInfo cl) {
    for (FieldInfo field : fields) {
      if (!field.containingClass().qualifiedName().contains(".internal.")) {
        if (!is_first_member) {
          buf.append(",");
        }
        buf.append("\n      \"" + field.name() + "\": \"");
        if (!field.containingClass().qualifiedName().equals(cl.qualifiedName())) {
          buf.append(field.containingClass().qualifiedName());
        }
        buf.append("\"");
        is_first_member = false;
      }
    }
    return is_first_member;
  }

  /**
  * Append the provided string builder with the navtree info for the provided list of methods.
  *
  * @param buf The string builder to append to.
  * @param methods A list of MethodInfo objects. Navtree info for each method will be appended
  *        to the provided string builder.
  * @param is_first_member True if this is the first child member listed under the parent class.
  * @param cl The ClassInfo object for the parent class of this method.
  */
  private static boolean addMethods(StringBuilder buf, ArrayList<MethodInfo> methods,
      boolean is_first_member, ClassInfo cl) {
    for (MethodInfo method : methods) {
      if (!method.containingClass().qualifiedName().contains(".internal.")) {
        if (!is_first_member) {
          buf.append(",");
        }
        buf.append("\n      \"" + method.name() + method.signature() + "\": \"");
        if (!method.containingClass().qualifiedName().equals(cl.qualifiedName())) {
          buf.append(method.containingClass().qualifiedName());
        }
        buf.append("\"");
        is_first_member = false;
      }
    }
    return is_first_member;
  }

}
