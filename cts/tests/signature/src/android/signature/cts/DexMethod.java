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
package android.signature.cts;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class DexMethod extends DexMember {
  private final List<String> mParamTypeList;

  public DexMethod(String className, String name, String signature) {
      super(className, name, parseDexReturnType(signature));
      mParamTypeList = parseDexTypeList(signature);
  }

  public String getDexSignature() {
      return "(" + String.join("", mParamTypeList) + ")" + getDexType();
  }

  public List<String> getJavaParameterTypes() {
      return mParamTypeList.stream().map(DexMember::dexToJavaType).collect(Collectors.toList());
  }

  public boolean isConstructor() {
      return "<init>".equals(getName()) && "V".equals(getDexType());
  }

  public boolean isStaticConstructor() {
      return "<clinit>".equals(getName()) && "V".equals(getDexType());
  }

  @Override
  public String toString() {
      return getJavaType() + " " + getJavaClassName() + "." + getName()
              + "(" + String.join(", ", getJavaParameterTypes()) + ")";
  }

  private static Matcher matchSignature(String signature) {
      Matcher m = Pattern.compile("^\\((.*)\\)(.*)$").matcher(signature);
      if (!m.matches()) {
          throw new RuntimeException("Could not parse method signature: " + signature);
      }
      return m;
  }

  private static String parseDexReturnType(String signature) {
      return matchSignature(signature).group(2);
  }

  private static List<String> parseDexTypeList(String signature) {
      String typeSequence = matchSignature(signature).group(1);
      List<String> list = new ArrayList<String>();
      while (!typeSequence.isEmpty()) {
          String type = firstDexTypeFromList(typeSequence);
          list.add(type);
          typeSequence = typeSequence.substring(type.length());
      }
      return list;
  }

  /**
   * Returns the first dex type in `typeList` or throws a ParserException
   * if a dex type is not recognized. The input is not changed.
   */
  private static String firstDexTypeFromList(String typeList) {
      String dexDimension = "";
      while (typeList.startsWith("[")) {
          dexDimension += "[";
          typeList = typeList.substring(1);
      }

      String type = null;
      if (typeList.startsWith("V")
              || typeList.startsWith("Z")
              || typeList.startsWith("B")
              || typeList.startsWith("C")
              || typeList.startsWith("S")
              || typeList.startsWith("I")
              || typeList.startsWith("J")
              || typeList.startsWith("F")
              || typeList.startsWith("D")) {
          type = typeList.substring(0, 1);
      } else if (typeList.startsWith("L") && typeList.indexOf(";") > 0) {
          type = typeList.substring(0, typeList.indexOf(";") + 1);
      } else {
          throw new RuntimeException("Unexpected dex type in \"" + typeList + "\"");
      }

      return dexDimension + type;
  }
}
