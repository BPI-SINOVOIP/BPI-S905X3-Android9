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

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;
import java.util.function.Predicate;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class Stubs {
  public static void writeStubsAndApi(String stubsDir, String apiFile, String dexApiFile,
      String keepListFile, String removedApiFile, String removedDexApiFile, String exactApiFile,
      String privateApiFile, String privateDexApiFile, HashSet<String> stubPackages,
      HashSet<String> stubImportPackages, boolean stubSourceOnly) {
    // figure out which classes we need
    final HashSet<ClassInfo> notStrippable = new HashSet<ClassInfo>();
    Collection<ClassInfo> all = Converter.allClasses();
    Map<PackageInfo, List<ClassInfo>> allClassesByPackage = null;
    PrintStream apiWriter = null;
    PrintStream dexApiWriter = null;
    PrintStream keepListWriter = null;
    PrintStream removedApiWriter = null;
    PrintStream removedDexApiWriter = null;
    PrintStream exactApiWriter = null;
    PrintStream privateApiWriter = null;
    PrintStream privateDexApiWriter = null;

    if (apiFile != null) {
      try {
        File xml = new File(apiFile);
        xml.getParentFile().mkdirs();
        apiWriter = new PrintStream(new BufferedOutputStream(new FileOutputStream(xml)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(apiFile, 0, 0),
            "Cannot open file for write.");
      }
    }
    if (dexApiFile != null) {
      try {
        File dexApi = new File(dexApiFile);
        dexApi.getParentFile().mkdirs();
        dexApiWriter = new PrintStream(new BufferedOutputStream(new FileOutputStream(dexApi)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(dexApiFile, 0, 0),
            "Cannot open file for write.");
      }
    }
    if (keepListFile != null) {
      try {
        File keepList = new File(keepListFile);
        keepList.getParentFile().mkdirs();
        keepListWriter = new PrintStream(new BufferedOutputStream(new FileOutputStream(keepList)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(keepListFile, 0, 0),
            "Cannot open file for write.");
      }
    }
    if (removedApiFile != null) {
      try {
        File removedApi = new File(removedApiFile);
        removedApi.getParentFile().mkdirs();
        removedApiWriter = new PrintStream(
            new BufferedOutputStream(new FileOutputStream(removedApi)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(removedApiFile, 0, 0),
            "Cannot open file for write");
      }
    }
    if (removedDexApiFile != null) {
      try {
        File removedDexApi = new File(removedDexApiFile);
        removedDexApi.getParentFile().mkdirs();
        removedDexApiWriter = new PrintStream(
            new BufferedOutputStream(new FileOutputStream(removedDexApi)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(removedDexApiFile, 0, 0),
            "Cannot open file for write");
      }
    }
    if (exactApiFile != null) {
      try {
        File exactApi = new File(exactApiFile);
        exactApi.getParentFile().mkdirs();
        exactApiWriter = new PrintStream(
            new BufferedOutputStream(new FileOutputStream(exactApi)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(exactApiFile, 0, 0),
            "Cannot open file for write");
      }
    }
    if (privateApiFile != null) {
      try {
        File privateApi = new File(privateApiFile);
        privateApi.getParentFile().mkdirs();
        privateApiWriter = new PrintStream(
            new BufferedOutputStream(new FileOutputStream(privateApi)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(privateApiFile, 0, 0),
            "Cannot open file for write");
      }
    }
    if (privateDexApiFile != null) {
      try {
        File privateDexApi = new File(privateDexApiFile);
        privateDexApi.getParentFile().mkdirs();
        privateDexApiWriter = new PrintStream(
            new BufferedOutputStream(new FileOutputStream(privateDexApi)));
      } catch (FileNotFoundException e) {
        Errors.error(Errors.IO_ERROR, new SourcePositionInfo(privateDexApiFile, 0, 0),
            "Cannot open file for write");
      }
    }
    // If a class is public or protected, not hidden, not imported and marked as included,
    // then we can't strip it
    for (ClassInfo cl : all) {
      if (cl.checkLevel() && cl.isIncluded()) {
        cantStripThis(cl, notStrippable, "0:0", stubImportPackages);
      }
    }

    // complain about anything that looks includeable but is not supposed to
    // be written, e.g. hidden things
    for (ClassInfo cl : notStrippable) {
      if (!cl.isHiddenOrRemoved()) {
        for (MethodInfo m : cl.selfMethods()) {
          if (m.isHiddenOrRemoved()) {
            Errors.error(Errors.UNAVAILABLE_SYMBOL, m.position(), "Reference to unavailable method "
                + m.name());
          } else if (m.isDeprecated()) {
            // don't bother reporting deprecated methods
            // unless they are public
            Errors.error(Errors.DEPRECATED, m.position(), "Method " + cl.qualifiedName() + "."
                + m.name() + " is deprecated");
          }

          ClassInfo hiddenClass = findHiddenClasses(m.returnType(), stubImportPackages);
          if (null != hiddenClass) {
            if (hiddenClass.qualifiedName() == m.returnType().asClassInfo().qualifiedName()) {
              // Return type is hidden
              Errors.error(Errors.UNAVAILABLE_SYMBOL, m.position(), "Method " + cl.qualifiedName()
                  + "." + m.name() + " returns unavailable type " + hiddenClass.name());
            } else {
              // Return type contains a generic parameter
              Errors.error(Errors.HIDDEN_TYPE_PARAMETER, m.position(), "Method " + cl.qualifiedName()
                  + "." + m.name() + " returns unavailable type " + hiddenClass.name()
                  + " as a type parameter");
            }
          }

          for (ParameterInfo p :  m.parameters()) {
            TypeInfo t = p.type();
            if (!t.isPrimitive()) {
              hiddenClass = findHiddenClasses(t, stubImportPackages);
              if (null != hiddenClass) {
                if (hiddenClass.qualifiedName() == t.asClassInfo().qualifiedName()) {
                  // Parameter type is hidden
                  Errors.error(Errors.UNAVAILABLE_SYMBOL, m.position(),
                      "Parameter of unavailable type " + t.fullName() + " in " + cl.qualifiedName()
                      + "." + m.name() + "()");
                } else {
                  // Parameter type contains a generic parameter
                  Errors.error(Errors.HIDDEN_TYPE_PARAMETER, m.position(),
                      "Parameter uses type parameter of unavailable type " + t.fullName() + " in "
                      + cl.qualifiedName() + "." + m.name() + "()");
                }
              }
            }
          }
        }

        // annotations are handled like methods
        for (MethodInfo m : cl.annotationElements()) {
          if (m.isHiddenOrRemoved()) {
            Errors.error(Errors.UNAVAILABLE_SYMBOL, m.position(), "Reference to unavailable annotation "
                + m.name());
          }

          ClassInfo returnClass = m.returnType().asClassInfo();
          if (returnClass != null && returnClass.isHiddenOrRemoved()) {
            Errors.error(Errors.UNAVAILABLE_SYMBOL, m.position(), "Annotation '" + m.name()
                + "' returns unavailable type " + returnClass.name());
          }

          for (ParameterInfo p :  m.parameters()) {
            TypeInfo t = p.type();
            if (!t.isPrimitive()) {
              if (t.asClassInfo().isHiddenOrRemoved()) {
                Errors.error(Errors.UNAVAILABLE_SYMBOL, p.position(),
                    "Reference to unavailable annotation class " + t.fullName());
              }
            }
          }
        }
      } else if (cl.isDeprecated()) {
        // not hidden, but deprecated
        Errors.error(Errors.DEPRECATED, cl.position(), "Class " + cl.qualifiedName()
            + " is deprecated");
      }
    }

    // packages contains all the notStrippable classes mapped by their containing packages
    HashMap<PackageInfo, List<ClassInfo>> packages = new HashMap<PackageInfo, List<ClassInfo>>();
    final HashSet<Pattern> stubPackageWildcards = extractWildcards(stubPackages);
    for (ClassInfo cl : notStrippable) {
      if (!cl.isDocOnly()) {
        if (stubSourceOnly && !Files.exists(Paths.get(cl.position().file))) {
          continue;
        }
        if (shouldWriteStub(cl.containingPackage().name(), stubPackages, stubPackageWildcards)) {
          // write out the stubs
          if (stubsDir != null) {
            writeClassFile(stubsDir, notStrippable, cl);
          }
          // build class list for api file or keep list file
          if (apiWriter != null || dexApiWriter != null || keepListWriter != null) {
            if (packages.containsKey(cl.containingPackage())) {
              packages.get(cl.containingPackage()).add(cl);
            } else {
              ArrayList<ClassInfo> classes = new ArrayList<ClassInfo>();
              classes.add(cl);
              packages.put(cl.containingPackage(), classes);
            }
          }
        }
      }
    }

    if (privateApiWriter != null || privateDexApiWriter != null || removedApiWriter != null
            || removedDexApiWriter != null) {
      allClassesByPackage = Converter.allClasses().stream()
          // Make sure that the files only contains information from the required packages.
          .filter(ci -> stubPackages == null
              || stubPackages.contains(ci.containingPackage().qualifiedName()))
          .collect(Collectors.groupingBy(ClassInfo::containingPackage));
    }

    final boolean ignoreShown = Doclava.showUnannotated;

    Predicate<MemberInfo> memberIsNotCloned = (x -> !x.isCloned());

    FilterPredicate apiFilter = new FilterPredicate(new ApiPredicate().setIgnoreShown(ignoreShown));
    ApiPredicate apiReference = new ApiPredicate().setIgnoreShown(true);
    Predicate<MemberInfo> apiEmit = apiFilter.and(new ElidingPredicate(apiReference));
    Predicate<MemberInfo> dexApiEmit = memberIsNotCloned.and(apiFilter);

    Predicate<MemberInfo> privateEmit = memberIsNotCloned.and(apiFilter.negate());
    Predicate<MemberInfo> privateReference = (x -> true);

    FilterPredicate removedFilter =
        new FilterPredicate(new ApiPredicate().setIgnoreShown(ignoreShown).setMatchRemoved(true));
    ApiPredicate removedReference = new ApiPredicate().setIgnoreShown(true).setIgnoreRemoved(true);
    Predicate<MemberInfo> removedEmit = removedFilter.and(new ElidingPredicate(removedReference));
    Predicate<MemberInfo> removedDexEmit = memberIsNotCloned.and(removedFilter);

    // Write out the current API
    if (apiWriter != null) {
      writeApi(apiWriter, packages, apiEmit, apiReference);
      apiWriter.close();
    }

    // Write out the current DEX API
    if (dexApiWriter != null) {
      writeDexApi(dexApiWriter, packages, dexApiEmit);
      dexApiWriter.close();
    }

    // Write out the keep list
    if (keepListWriter != null) {
      writeKeepList(keepListWriter, packages, notStrippable);
      keepListWriter.close();
    }

    // Write out the private API
    if (privateApiWriter != null) {
      writeApi(privateApiWriter, allClassesByPackage, privateEmit, privateReference);
      privateApiWriter.close();
    }

    // Write out the private API
    if (privateDexApiWriter != null) {
      writeDexApi(privateDexApiWriter, allClassesByPackage, privateEmit);
      privateDexApiWriter.close();
    }

    // Write out the removed API
    if (removedApiWriter != null) {
      writeApi(removedApiWriter, allClassesByPackage, removedEmit, removedReference);
      removedApiWriter.close();
    }

    // Write out the removed DEX API
    if (removedDexApiWriter != null) {
      writeDexApi(removedDexApiWriter, allClassesByPackage, removedDexEmit);
      removedDexApiWriter.close();
    }
  }

  private static boolean shouldWriteStub(final String packageName,
          final HashSet<String> stubPackages, final HashSet<Pattern> stubPackageWildcards) {
    if (stubPackages == null) {
      // There aren't any stub packages set, write all stubs
      return true;
    }
    if (stubPackages.contains(packageName)) {
      // Stub packages contains package, return true
      return true;
    }
    if (stubPackageWildcards != null) {
      // Else, we will iterate through the wildcards to see if there's a match
      for (Pattern wildcard : stubPackageWildcards) {
        if (wildcard.matcher(packageName).matches()) {
          return true;
        }
      }
    }
    return false;
  }

  private static HashSet<Pattern> extractWildcards(HashSet<String> stubPackages) {
    HashSet<Pattern> wildcards = null;
    if (stubPackages != null) {
      for (Iterator<String> i = stubPackages.iterator(); i.hasNext();) {
        final String pkg = i.next();
        if (pkg.indexOf('*') != -1) {
          if (wildcards == null) {
            wildcards = new HashSet<Pattern>();
          }
          // Add the compiled wildcard, replacing * with the regex equivalent
          wildcards.add(Pattern.compile(pkg.replace("*", ".*?")));
          // And remove the raw wildcard from the packages
          i.remove();
        }
      }
    }
    return wildcards;
  }

  /**
   * Find references to hidden classes.
   *
   * <p>This finds hidden classes that are used by public parts of the API in order to ensure the
   * API is self consistent and does not reference classes that are not included in
   * the stubs. Any such references cause an error to be reported.
   *
   * <p>A reference to an imported class is not treated as an error, even though imported classes
   * are hidden from the stub generation. That is because imported classes are, by definition,
   * excluded from the set of classes for which stubs are required.
   *
   * @param ti the type information to examine for references to hidden classes.
   * @param stubImportPackages the possibly null set of imported package names.
   * @return a reference to a hidden class or null if there are none
   */
  private static ClassInfo findHiddenClasses(TypeInfo ti, HashSet<String> stubImportPackages) {
    ClassInfo ci = ti.asClassInfo();
    if (ci == null) return null;
    if (stubImportPackages != null
        && stubImportPackages.contains(ci.containingPackage().qualifiedName())) {
      return null;
    }
    if (ci.isHiddenOrRemoved()) return ci;
    if (ti.typeArguments() != null) {
      for (TypeInfo tii : ti.typeArguments()) {
        // Avoid infinite recursion in the case of Foo<T extends Foo>
        if (tii.qualifiedTypeName() != ti.qualifiedTypeName()) {
          ClassInfo hiddenClass = findHiddenClasses(tii, stubImportPackages);
          if (hiddenClass != null) return hiddenClass;
        }
      }
    }
    return null;
  }

  public static void cantStripThis(ClassInfo cl, HashSet<ClassInfo> notStrippable, String why,
      HashSet<String> stubImportPackages) {

    if (stubImportPackages != null
        && stubImportPackages.contains(cl.containingPackage().qualifiedName())) {
      // if the package is imported then it does not need stubbing.
      return;
    }

    if (!notStrippable.add(cl)) {
      // slight optimization: if it already contains cl, it already contains
      // all of cl's parents
      return;
    }
    cl.setReasonIncluded(why);

    // cant strip annotations
    /*
     * if (cl.annotations() != null){ for (AnnotationInstanceInfo ai : cl.annotations()){ if
     * (ai.type() != null){ cantStripThis(ai.type(), notStrippable, "1:" + cl.qualifiedName()); } }
     * }
     */
    // cant strip any public fields or their generics
    if (cl.selfFields() != null) {
      for (FieldInfo fInfo : cl.selfFields()) {
        if (fInfo.type() != null) {
          if (fInfo.type().asClassInfo() != null) {
            cantStripThis(fInfo.type().asClassInfo(), notStrippable, "2:" + cl.qualifiedName(),
                stubImportPackages);
          }
          if (fInfo.type().typeArguments() != null) {
            for (TypeInfo tTypeInfo : fInfo.type().typeArguments()) {
              if (tTypeInfo.asClassInfo() != null) {
                cantStripThis(tTypeInfo.asClassInfo(), notStrippable, "3:" + cl.qualifiedName(),
                    stubImportPackages);
              }
            }
          }
        }
      }
    }
    // cant strip any of the type's generics
    if (cl.asTypeInfo() != null) {
      if (cl.asTypeInfo().typeArguments() != null) {
        for (TypeInfo tInfo : cl.asTypeInfo().typeArguments()) {
          if (tInfo.asClassInfo() != null) {
            cantStripThis(tInfo.asClassInfo(), notStrippable, "4:" + cl.qualifiedName(),
                stubImportPackages);
          }
        }
      }
    }
    // cant strip any of the annotation elements
    // cantStripThis(cl.annotationElements(), notStrippable);
    // take care of methods
    cantStripThis(cl.allSelfMethods(), notStrippable, stubImportPackages);
    cantStripThis(cl.allConstructors(), notStrippable, stubImportPackages);
    // blow the outer class open if this is an inner class
    if (cl.containingClass() != null) {
      cantStripThis(cl.containingClass(), notStrippable, "5:" + cl.qualifiedName(),
          stubImportPackages);
    }
    // blow open super class and interfaces
    ClassInfo supr = cl.realSuperclass();
    if (supr != null) {
      if (supr.isHiddenOrRemoved()) {
        // cl is a public class declared as extending a hidden superclass.
        // this is not a desired practice but it's happened, so we deal
        // with it by finding the first super class which passes checklevel for purposes of
        // generating the doc & stub information, and proceeding normally.
        ClassInfo publicSuper = cl.superclass();
        cl.init(cl.asTypeInfo(), cl.realInterfaces(), cl.realInterfaceTypes(), cl.innerClasses(),
            cl.allConstructors(), cl.allSelfMethods(), cl.annotationElements(), cl.allSelfFields(),
            cl.enumConstants(), cl.containingPackage(), cl.containingClass(),
            publicSuper, publicSuper.asTypeInfo(), cl.annotations());
        Errors.error(Errors.HIDDEN_SUPERCLASS, cl.position(), "Public class " + cl.qualifiedName()
            + " stripped of unavailable superclass " + supr.qualifiedName());
      } else {
        cantStripThis(supr, notStrippable, "6:" + cl.realSuperclass().name() + cl.qualifiedName(),
            stubImportPackages);
        if (supr.isPrivate()) {
          Errors.error(Errors.PRIVATE_SUPERCLASS, cl.position(), "Public class "
              + cl.qualifiedName() + " extends private class " + supr.qualifiedName());
        }
      }
    }
  }

  private static void cantStripThis(ArrayList<MethodInfo> mInfos, HashSet<ClassInfo> notStrippable,
      HashSet<String> stubImportPackages) {
    // for each method, blow open the parameters, throws and return types. also blow open their
    // generics
    if (mInfos != null) {
      for (MethodInfo mInfo : mInfos) {
        if (mInfo.getTypeParameters() != null) {
          for (TypeInfo tInfo : mInfo.getTypeParameters()) {
            if (tInfo.asClassInfo() != null) {
              cantStripThis(tInfo.asClassInfo(), notStrippable, "8:"
                  + mInfo.realContainingClass().qualifiedName() + ":" + mInfo.name(),
                  stubImportPackages);
            }
          }
        }
        if (mInfo.parameters() != null) {
          for (ParameterInfo pInfo : mInfo.parameters()) {
            if (pInfo.type() != null && pInfo.type().asClassInfo() != null) {
              cantStripThis(pInfo.type().asClassInfo(), notStrippable, "9:"
                  + mInfo.realContainingClass().qualifiedName() + ":" + mInfo.name(),
                  stubImportPackages);
              if (pInfo.type().typeArguments() != null) {
                for (TypeInfo tInfoType : pInfo.type().typeArguments()) {
                  if (tInfoType.asClassInfo() != null) {
                    ClassInfo tcl = tInfoType.asClassInfo();
                    if (tcl.isHiddenOrRemoved()) {
                      Errors
                          .error(Errors.UNAVAILABLE_SYMBOL, mInfo.position(),
                              "Parameter of hidden type " + tInfoType.fullName() + " in "
                                  + mInfo.containingClass().qualifiedName() + '.' + mInfo.name()
                                  + "()");
                    } else {
                      cantStripThis(tcl, notStrippable, "10:"
                          + mInfo.realContainingClass().qualifiedName() + ":" + mInfo.name(),
                          stubImportPackages);
                    }
                  }
                }
              }
            }
          }
        }
        for (ClassInfo thrown : mInfo.thrownExceptions()) {
          cantStripThis(thrown, notStrippable, "11:" + mInfo.realContainingClass().qualifiedName()
              + ":" + mInfo.name(), stubImportPackages);
        }
        if (mInfo.returnType() != null && mInfo.returnType().asClassInfo() != null) {
          cantStripThis(mInfo.returnType().asClassInfo(), notStrippable, "12:"
              + mInfo.realContainingClass().qualifiedName() + ":" + mInfo.name(),
              stubImportPackages);
          if (mInfo.returnType().typeArguments() != null) {
            for (TypeInfo tyInfo : mInfo.returnType().typeArguments()) {
              if (tyInfo.asClassInfo() != null) {
                cantStripThis(tyInfo.asClassInfo(), notStrippable, "13:"
                    + mInfo.realContainingClass().qualifiedName() + ":" + mInfo.name(),
                    stubImportPackages);
              }
            }
          }
        }
      }
    }
  }

  static String javaFileName(ClassInfo cl) {
    String dir = "";
    PackageInfo pkg = cl.containingPackage();
    if (pkg != null) {
      dir = pkg.name();
      dir = dir.replace('.', '/') + '/';
    }
    return dir + cl.name() + ".java";
  }

  static void writeClassFile(String stubsDir, HashSet<ClassInfo> notStrippable, ClassInfo cl) {
    // inner classes are written by their containing class
    if (cl.containingClass() != null) {
      return;
    }

    // Work around the bogus "Array" class we invent for
    // Arrays.copyOf's Class<? extends T[]> newType parameter. (http://b/2715505)
    if (cl.containingPackage() != null
        && cl.containingPackage().name().equals(PackageInfo.DEFAULT_PACKAGE)) {
      return;
    }

    String filename = stubsDir + '/' + javaFileName(cl);
    File file = new File(filename);
    ClearPage.ensureDirectory(file);

    PrintStream stream = null;
    try {
      stream = new PrintStream(new BufferedOutputStream(new FileOutputStream(file)));
      writeClassFile(stream, notStrippable, cl);
    } catch (FileNotFoundException e) {
      System.err.println("error writing file: " + filename);
    } finally {
      if (stream != null) {
        stream.close();
      }
    }
  }

  static void writeClassFile(PrintStream stream, HashSet<ClassInfo> notStrippable, ClassInfo cl) {
    PackageInfo pkg = cl.containingPackage();
    if (cl.containingClass() == null) {
        stream.print(parseLicenseHeader(cl.position()));
    }
    if (pkg != null) {
      stream.println("package " + pkg.name() + ";");
    }
    writeClass(stream, notStrippable, cl);
  }

  private static String parseLicenseHeader(/* @Nonnull */ SourcePositionInfo positionInfo) {
    if (positionInfo == null) {
      throw new NullPointerException("positionInfo == null");
    }

    try {
      final File sourceFile = new File(positionInfo.file);
      if (!sourceFile.exists()) {
        throw new IllegalArgumentException("Unable to find " + sourceFile +
                ". This is usually because doclava has been asked to generate stubs for a file " +
                "that isn't present in the list of input source files but exists in the input " +
                "classpath.");
      }
      return parseLicenseHeader(new FileInputStream(sourceFile));
    } catch (IOException ioe) {
      throw new RuntimeException("Unable to parse license header for: " + positionInfo.file, ioe);
    }
  }

  /* @VisibleForTesting */
  static String parseLicenseHeader(InputStream input) throws IOException {
    StringBuilder builder = new StringBuilder(8192);
    try (Scanner scanner  = new Scanner(
          new BufferedReader(new InputStreamReader(input, StandardCharsets.UTF_8)))) {
      String line;
      while (scanner.hasNextLine()) {
        line = scanner.nextLine().trim();
        // Use an extremely simple strategy for parsing license headers : assume that
        // all file content before the first "package " or "import " directive is a license
        // header. In some cases this might contain more than just the license header, but we
        // don't care.
        if (line.startsWith("package ") || line.startsWith("import ")) {
          break;
        }
        builder.append(line);
        builder.append("\n");
      }

      // We've reached the end of the file without reaching any package or import
      // directives.
      if (!scanner.hasNextLine()) {
        throw new IOException("Unable to parse license header");
      }
    }

    return builder.toString();
  }

  static void writeClass(PrintStream stream, HashSet<ClassInfo> notStrippable, ClassInfo cl) {
    writeAnnotations(stream, cl.annotations(), cl.isDeprecated());

    stream.print(cl.scope() + " ");
    if (cl.isAbstract() && !cl.isAnnotation() && !cl.isInterface()) {
      stream.print("abstract ");
    }
    if (cl.isStatic()) {
      stream.print("static ");
    }
    if (cl.isFinal() && !cl.isEnum()) {
      stream.print("final ");
    }
    if (false) {
      stream.print("strictfp ");
    }

    HashSet<String> classDeclTypeVars = new HashSet();
    String leafName = cl.asTypeInfo().fullName(classDeclTypeVars);
    int bracket = leafName.indexOf('<');
    if (bracket < 0) bracket = leafName.length() - 1;
    int period = leafName.lastIndexOf('.', bracket);
    if (period < 0) period = -1;
    leafName = leafName.substring(period + 1);

    String kind = cl.kind();
    stream.println(kind + " " + leafName);

    TypeInfo base = cl.superclassType();

    if (!"enum".equals(kind)) {
      if (base != null && !"java.lang.Object".equals(base.qualifiedTypeName())) {
        stream.println("  extends " + base.fullName(classDeclTypeVars));
      }
    }

    List<TypeInfo> usedInterfaces = new ArrayList<TypeInfo>();
    for (TypeInfo iface : cl.realInterfaceTypes()) {
      if (notStrippable.contains(iface.asClassInfo()) && !iface.asClassInfo().isDocOnly()) {
        usedInterfaces.add(iface);
      }
    }
    if (usedInterfaces.size() > 0 && !cl.isAnnotation()) {
      // can java annotations extend other ones?
      if (cl.isInterface() || cl.isAnnotation()) {
        stream.print("  extends ");
      } else {
        stream.print("  implements ");
      }
      String comma = "";
      for (TypeInfo iface : usedInterfaces) {
        stream.print(comma + iface.fullName(classDeclTypeVars));
        comma = ", ";
      }
      stream.println();
    }

    stream.println("{");

    ArrayList<FieldInfo> enumConstants = cl.enumConstants();
    int N = enumConstants.size();
    int i = 0;
    for (FieldInfo field : enumConstants) {
      writeAnnotations(stream, field.annotations(), field.isDeprecated());
      if (!field.constantLiteralValue().equals("null")) {
        stream.println(field.name() + "(" + field.constantLiteralValue()
            + (i == N - 1 ? ");" : "),"));
      } else {
        stream.println(field.name() + "(" + (i == N - 1 ? ");" : "),"));
      }
      i++;
    }

    for (ClassInfo inner : cl.getRealInnerClasses()) {
      if (notStrippable.contains(inner) && !inner.isDocOnly()) {
        writeClass(stream, notStrippable, inner);
      }
    }


    for (MethodInfo method : cl.constructors()) {
      if (!method.isDocOnly()) {
        writeMethod(stream, method, true);
      }
    }

    boolean fieldNeedsInitialization = false;
    boolean staticFieldNeedsInitialization = false;
    for (FieldInfo field : cl.selfFields()) {
      if (!field.isDocOnly()) {
        if (!field.isStatic() && field.isFinal() && !fieldIsInitialized(field)) {
          fieldNeedsInitialization = true;
        }
        if (field.isStatic() && field.isFinal() && !fieldIsInitialized(field)) {
          staticFieldNeedsInitialization = true;
        }
      }
    }

    // The compiler includes a default public constructor that calls the super classes
    // default constructor in the case where there are no written constructors.
    // So, if we hide all the constructors, java may put in a constructor
    // that calls a nonexistent super class constructor. So, if there are no constructors,
    // and the super class doesn't have a default constructor, write in a private constructor
    // that works. TODO -- we generate this as protected, but we really should generate
    // it as private unless it also exists in the real code.
    if ((cl.constructors().isEmpty() && (!cl.getNonWrittenConstructors().isEmpty() ||
        fieldNeedsInitialization)) && !cl.isAnnotation() && !cl.isInterface() && !cl.isEnum()) {
      // Errors.error(Errors.HIDDEN_CONSTRUCTOR,
      // cl.position(), "No constructors " +
      // "found and superclass has no parameterless constructor.  A constructor " +
      // "that calls an appropriate superclass constructor " +
      // "was automatically written to stubs.\n");
      stream.println(cl.leafName() + "() { " + superCtorCall(cl, null) + "throw new"
          + " RuntimeException(\"Stub!\"); }");
    }

    for (MethodInfo method : cl.allSelfMethods()) {
      if (cl.isEnum()) {
        if (("values".equals(method.name()) && "()".equals(method.signature())) ||
            ("valueOf".equals(method.name()) &&
            "(java.lang.String)".equals(method.signature()))) {
          // skip these two methods on enums, because they're synthetic,
          // although for some reason javadoc doesn't mark them as synthetic,
          // maybe because they still want them documented
          continue;
        }
      }
      if (!method.isDocOnly()) {
        writeMethod(stream, method, false);
      }
    }
    // Write all methods that are hidden or removed, but override abstract methods or interface methods.
    // These can't be hidden.
    List<MethodInfo> hiddenAndRemovedMethods = cl.getHiddenMethods();
    hiddenAndRemovedMethods.addAll(cl.getRemovedMethods());
    for (MethodInfo method : hiddenAndRemovedMethods) {
      MethodInfo overriddenMethod =
          method.findRealOverriddenMethod(method.name(), method.signature(), notStrippable);
      ClassInfo classContainingMethod =
          method.findRealOverriddenClass(method.name(), method.signature());
      if (overriddenMethod != null && !overriddenMethod.isHiddenOrRemoved() &&
          !overriddenMethod.isDocOnly() &&
          (overriddenMethod.isAbstract() || overriddenMethod.containingClass().isInterface())) {
        method.setReason("1:" + classContainingMethod.qualifiedName());
        cl.addMethod(method);
        writeMethod(stream, method, false);
      }
    }

    for (MethodInfo element : cl.annotationElements()) {
      if (!element.isDocOnly()) {
        writeAnnotationElement(stream, element);
      }
    }

    for (FieldInfo field : cl.selfFields()) {
      if (!field.isDocOnly()) {
        writeField(stream, field);
      }
    }

    if (staticFieldNeedsInitialization) {
      stream.print("static { ");
      for (FieldInfo field : cl.selfFields()) {
        if (!field.isDocOnly() && field.isStatic() && field.isFinal() && !fieldIsInitialized(field)
            && field.constantValue() == null) {
          stream.print(field.name() + " = " + field.type().defaultValue() + "; ");
        }
      }
      stream.println("}");
    }

    stream.println("}");
  }

  static void writeMethod(PrintStream stream, MethodInfo method, boolean isConstructor) {
    String comma;

    writeAnnotations(stream, method.annotations(), method.isDeprecated());

    if (method.isDefault()) {
      stream.print("default ");
    }
    stream.print(method.scope() + " ");
    if (method.isStatic()) {
      stream.print("static ");
    }
    if (method.isFinal()) {
      stream.print("final ");
    }
    if (method.isAbstract()) {
      stream.print("abstract ");
    }
    if (method.isSynchronized()) {
      stream.print("synchronized ");
    }
    if (method.isNative()) {
      stream.print("native ");
    }
    if (false /* method.isStictFP() */) {
      stream.print("strictfp ");
    }

    stream.print(method.typeArgumentsName(new HashSet()) + " ");

    if (!isConstructor) {
      stream.print(method.returnType().fullName(method.typeVariables()) + " ");
    }
    String n = method.name();
    int pos = n.lastIndexOf('.');
    if (pos >= 0) {
      n = n.substring(pos + 1);
    }
    stream.print(n + "(");
    comma = "";
    int count = 1;
    int size = method.parameters().size();
    for (ParameterInfo param : method.parameters()) {
      stream.print(comma);
      writeAnnotations(stream, param.annotations(), false);
      stream.print(fullParameterTypeName(method, param.type(), count == size) + " "
          + param.name());
      comma = ", ";
      count++;
    }
    stream.print(")");

    comma = "";
    if (method.thrownExceptions().size() > 0) {
      stream.print(" throws ");
      for (ClassInfo thrown : method.thrownExceptions()) {
        stream.print(comma + thrown.qualifiedName());
        comma = ", ";
      }
    }
    if (method.isAbstract() || method.isNative() || (method.containingClass().isInterface() && (!method.isDefault() && !method.isStatic()))) {
      stream.println(";");
    } else {
      stream.print(" { ");
      if (isConstructor) {
        stream.print(superCtorCall(method.containingClass(), method.thrownExceptions()));
      }
      stream.println("throw new RuntimeException(\"Stub!\"); }");
    }
  }

  static void writeField(PrintStream stream, FieldInfo field) {
    writeAnnotations(stream, field.annotations(), field.isDeprecated());

    stream.print(field.scope() + " ");
    if (field.isStatic()) {
      stream.print("static ");
    }
    if (field.isFinal()) {
      stream.print("final ");
    }
    if (field.isTransient()) {
      stream.print("transient ");
    }
    if (field.isVolatile()) {
      stream.print("volatile ");
    }

    stream.print(field.type().fullName());
    stream.print(" ");
    stream.print(field.name());

    if (fieldIsInitialized(field)) {
      stream.print(" = " + field.constantLiteralValue());
    }

    stream.println(";");
  }

  static boolean fieldIsInitialized(FieldInfo field) {
    return (field.isFinal() && field.constantValue() != null)
        || !field.type().dimension().equals("") || field.containingClass().isInterface();
  }

  static boolean canCallMethod(ClassInfo from, MethodInfo m) {
    if (m.isPublic() || m.isProtected()) {
      return true;
    }
    if (m.isPackagePrivate()) {
      String fromPkg = from.containingPackage().name();
      String pkg = m.containingClass().containingPackage().name();
      if (fromPkg.equals(pkg)) {
        return true;
      }
    }
    return false;
  }

  // call a constructor, any constructor on this class's superclass.
  static String superCtorCall(ClassInfo cl, ArrayList<ClassInfo> thrownExceptions) {
    ClassInfo base = cl.realSuperclass();
    if (base == null) {
      return "";
    }
    HashSet<String> exceptionNames = new HashSet<String>();
    if (thrownExceptions != null) {
      for (ClassInfo thrown : thrownExceptions) {
        exceptionNames.add(thrown.name());
      }
    }
    ArrayList<MethodInfo> ctors = base.constructors();
    MethodInfo ctor = null;
    // bad exception indicates that the exceptions thrown by the super constructor
    // are incompatible with the constructor we're using for the sub class.
    Boolean badException = false;
    for (MethodInfo m : ctors) {
      if (canCallMethod(cl, m)) {
        if (m.thrownExceptions() != null) {
          for (ClassInfo thrown : m.thrownExceptions()) {
            if (thrownExceptions != null && !exceptionNames.contains(thrown.name())) {
              badException = true;
            }
          }
        }
        if (badException) {
          badException = false;
          continue;
        }
        // if it has no args, we're done
        if (m.parameters().isEmpty()) {
          return "";
        }
        ctor = m;
      }
    }
    if (ctor != null) {
      String result = "";
      result += "super(";
      ArrayList<ParameterInfo> params = ctor.parameters();
      for (ParameterInfo param : params) {
        TypeInfo t = param.type();
        if (t.isPrimitive() && t.dimension().equals("")) {
          String n = t.simpleTypeName();
          if (("byte".equals(n) || "short".equals(n) || "int".equals(n) || "long".equals(n)
              || "float".equals(n) || "double".equals(n))
              && t.dimension().equals("")) {
            result += "0";
          } else if ("char".equals(n)) {
            result += "'\\0'";
          } else if ("boolean".equals(n)) {
            result += "false";
          } else {
            result += "<<unknown-" + n + ">>";
          }
        } else {
          // put null in each super class method. Cast null to the correct type
          // to avoid collisions with other constructors. If the type is generic
          // don't cast it
          result +=
              (!t.isTypeVariable() ? "(" + t.qualifiedTypeName() + t.dimension() + ")" : "")
                  + "null";
        }
        if (param != params.get(params.size()-1)) {
          result += ",";
        }
      }
      result += "); ";
      return result;
    } else {
      return "";
    }
  }

    /**
     * Write out the given list of annotations. If the {@code isDeprecated}
     * flag is true also write out a {@code @Deprecated} annotation if it did not
     * already appear in the list of annotations. (This covers APIs that mention
     * {@code @deprecated} in their documentation but fail to add
     * {@code @Deprecated} as an annotation.
     * <p>
     * {@code @Override} annotations are deliberately skipped.
     */
  static void writeAnnotations(PrintStream stream, List<AnnotationInstanceInfo> annotations,
          boolean isDeprecated) {
    assert annotations != null;
    for (AnnotationInstanceInfo ann : annotations) {
      // Skip @Override annotations: the stubs do not need it and in some cases it leads
      // to compilation errors with the way the stubs are generated
      if (ann.type() != null && ann.type().qualifiedName().equals("java.lang.Override")) {
        continue;
      }
      if (!ann.type().isHiddenOrRemoved()) {
        stream.println(ann.toString());
        if (isDeprecated && ann.type() != null
            && ann.type().qualifiedName().equals("java.lang.Deprecated")) {
          isDeprecated = false; // Prevent duplicate annotations
        }
      }
    }
    if (isDeprecated) {
      stream.println("@Deprecated");
    }
  }

  static void writeAnnotationElement(PrintStream stream, MethodInfo ann) {
    stream.print(ann.returnType().fullName());
    stream.print(" ");
    stream.print(ann.name());
    stream.print("()");
    AnnotationValueInfo def = ann.defaultAnnotationElementValue();
    if (def != null) {
      stream.print(" default ");
      stream.print(def.valueString());
    }
    stream.println(";");
  }

  public static void writeXml(PrintStream xmlWriter, Collection<PackageInfo> pkgs, boolean strip) {
    if (strip) {
      Stubs.writeXml(xmlWriter, pkgs);
    } else {
      Stubs.writeXml(xmlWriter, pkgs, c -> true);
    }
  }

  public static void writeXml(PrintStream xmlWriter, Collection<PackageInfo> pkgs,
      Predicate<ClassInfo> notStrippable) {

    final PackageInfo[] packages = pkgs.toArray(new PackageInfo[pkgs.size()]);
    Arrays.sort(packages, PackageInfo.comparator);

    xmlWriter.println("<api>");
    for (PackageInfo pkg: packages) {
      writePackageXML(xmlWriter, pkg, pkg.allClasses().values(), notStrippable);
    }
    xmlWriter.println("</api>");
  }

  public static void writeXml(PrintStream xmlWriter, Collection<PackageInfo> pkgs) {
    HashSet<ClassInfo> allClasses = new HashSet<>();
    for (PackageInfo pkg: pkgs) {
      allClasses.addAll(pkg.allClasses().values());
    }
    Predicate<ClassInfo> notStrippable = allClasses::contains;
    writeXml(xmlWriter, pkgs, notStrippable);
  }

  static void writePackageXML(PrintStream xmlWriter, PackageInfo pack,
      Collection<ClassInfo> classList, Predicate<ClassInfo> notStrippable) {
    ClassInfo[] classes = classList.toArray(new ClassInfo[classList.size()]);
    Arrays.sort(classes, ClassInfo.comparator);
    // Work around the bogus "Array" class we invent for
    // Arrays.copyOf's Class<? extends T[]> newType parameter. (http://b/2715505)
    if (pack.name().equals(PackageInfo.DEFAULT_PACKAGE)) {
      return;
    }
    xmlWriter.println("<package name=\"" + pack.name() + "\"\n"
    // + " source=\"" + pack.position() + "\"\n"
        + ">");
    for (ClassInfo cl : classes) {
      writeClassXML(xmlWriter, cl, notStrippable);
    }
    xmlWriter.println("</package>");


  }

  static void writeClassXML(PrintStream xmlWriter, ClassInfo cl, Predicate<ClassInfo> notStrippable) {
    String scope = cl.scope();
    String deprecatedString = "";
    String declString = (cl.isInterface()) ? "interface" : "class";
    if (cl.isDeprecated()) {
      deprecatedString = "deprecated";
    } else {
      deprecatedString = "not deprecated";
    }
    xmlWriter.println("<" + declString + " name=\"" + cl.name() + "\"");
    if (!cl.isInterface() && !cl.qualifiedName().equals("java.lang.Object")) {
      xmlWriter.println(" extends=\""
          + ((cl.realSuperclass() == null) ? "java.lang.Object" : cl.realSuperclass()
              .qualifiedName()) + "\"");
    }
    xmlWriter.println(" abstract=\"" + cl.isAbstract() + "\"\n" + " static=\"" + cl.isStatic()
        + "\"\n" + " final=\"" + cl.isFinal() + "\"\n" + " deprecated=\"" + deprecatedString
        + "\"\n" + " visibility=\"" + scope + "\"\n"
        // + " source=\"" + cl.position() + "\"\n"
        + ">");

    ArrayList<ClassInfo> interfaces = cl.realInterfaces();
    Collections.sort(interfaces, ClassInfo.comparator);
    for (ClassInfo iface : interfaces) {
      if (notStrippable.test(iface)) {
        xmlWriter.println("<implements name=\"" + iface.qualifiedName() + "\">");
        xmlWriter.println("</implements>");
      }
    }

    ArrayList<MethodInfo> constructors = cl.constructors();
    Collections.sort(constructors, MethodInfo.comparator);
    for (MethodInfo mi : constructors) {
      writeConstructorXML(xmlWriter, mi);
    }

    ArrayList<MethodInfo> methods = cl.allSelfMethods();
    Collections.sort(methods, MethodInfo.comparator);
    for (MethodInfo mi : methods) {
      writeMethodXML(xmlWriter, mi);
    }

    ArrayList<FieldInfo> fields = cl.selfFields();
    Collections.sort(fields, FieldInfo.comparator);
    for (FieldInfo fi : fields) {
      writeFieldXML(xmlWriter, fi);
    }
    xmlWriter.println("</" + declString + ">");

  }

  static void writeMethodXML(PrintStream xmlWriter, MethodInfo mi) {
    String scope = mi.scope();

    String deprecatedString = "";
    if (mi.isDeprecated()) {
      deprecatedString = "deprecated";
    } else {
      deprecatedString = "not deprecated";
    }
    xmlWriter.println("<method name=\""
        + mi.name()
        + "\"\n"
        + ((mi.returnType() != null) ? " return=\""
            + makeXMLcompliant(fullParameterTypeName(mi, mi.returnType(), false)) + "\"\n" : "")
        + " abstract=\"" + mi.isAbstract() + "\"\n" + " native=\"" + mi.isNative() + "\"\n"
        + " synchronized=\"" + mi.isSynchronized() + "\"\n" + " static=\"" + mi.isStatic() + "\"\n"
        + " final=\"" + mi.isFinal() + "\"\n" + " deprecated=\"" + deprecatedString + "\"\n"
        + " visibility=\"" + scope + "\"\n"
        // + " source=\"" + mi.position() + "\"\n"
        + ">");

    // write parameters in declaration order
    int numParameters = mi.parameters().size();
    int count = 0;
    for (ParameterInfo pi : mi.parameters()) {
      count++;
      writeParameterXML(xmlWriter, mi, pi, count == numParameters);
    }

    // but write exceptions in canonicalized order
    ArrayList<ClassInfo> exceptions = mi.thrownExceptions();
    Collections.sort(exceptions, ClassInfo.comparator);
    for (ClassInfo pi : exceptions) {
      xmlWriter.println("<exception name=\"" + pi.name() + "\" type=\"" + pi.qualifiedName()
          + "\">");
      xmlWriter.println("</exception>");
    }
    xmlWriter.println("</method>");
  }

  static void writeConstructorXML(PrintStream xmlWriter, MethodInfo mi) {
    String scope = mi.scope();
    String deprecatedString = "";
    if (mi.isDeprecated()) {
      deprecatedString = "deprecated";
    } else {
      deprecatedString = "not deprecated";
    }
    xmlWriter.println("<constructor name=\"" + mi.name() + "\"\n" + " type=\""
        + mi.containingClass().qualifiedName() + "\"\n" + " static=\"" + mi.isStatic() + "\"\n"
        + " final=\"" + mi.isFinal() + "\"\n" + " deprecated=\"" + deprecatedString + "\"\n"
        + " visibility=\"" + scope + "\"\n"
        // + " source=\"" + mi.position() + "\"\n"
        + ">");

    int numParameters = mi.parameters().size();
    int count = 0;
    for (ParameterInfo pi : mi.parameters()) {
      count++;
      writeParameterXML(xmlWriter, mi, pi, count == numParameters);
    }

    ArrayList<ClassInfo> exceptions = mi.thrownExceptions();
    Collections.sort(exceptions, ClassInfo.comparator);
    for (ClassInfo pi : exceptions) {
      xmlWriter.println("<exception name=\"" + pi.name() + "\" type=\"" + pi.qualifiedName()
          + "\">");
      xmlWriter.println("</exception>");
    }
    xmlWriter.println("</constructor>");
  }

  static void writeParameterXML(PrintStream xmlWriter, MethodInfo method, ParameterInfo pi,
      boolean isLast) {
    xmlWriter.println("<parameter name=\"" + pi.name() + "\" type=\""
        + makeXMLcompliant(fullParameterTypeName(method, pi.type(), isLast)) + "\">");
    xmlWriter.println("</parameter>");
  }

  static void writeFieldXML(PrintStream xmlWriter, FieldInfo fi) {
    String scope = fi.scope();
    String deprecatedString = "";
    if (fi.isDeprecated()) {
      deprecatedString = "deprecated";
    } else {
      deprecatedString = "not deprecated";
    }
    // need to make sure value is valid XML
    String value = makeXMLcompliant(fi.constantLiteralValue());

    String fullTypeName = makeXMLcompliant(fi.type().fullName());

    xmlWriter.println("<field name=\"" + fi.name() + "\"\n" + " type=\"" + fullTypeName + "\"\n"
        + " transient=\"" + fi.isTransient() + "\"\n" + " volatile=\"" + fi.isVolatile() + "\"\n"
        + (fieldIsInitialized(fi) ? " value=\"" + value + "\"\n" : "") + " static=\""
        + fi.isStatic() + "\"\n" + " final=\"" + fi.isFinal() + "\"\n" + " deprecated=\""
        + deprecatedString + "\"\n" + " visibility=\"" + scope + "\"\n"
        // + " source=\"" + fi.position() + "\"\n"
        + ">");
    xmlWriter.println("</field>");
  }

  static String makeXMLcompliant(String s) {
    String returnString = "";
    returnString = s.replaceAll("&", "&amp;");
    returnString = returnString.replaceAll("<", "&lt;");
    returnString = returnString.replaceAll(">", "&gt;");
    returnString = returnString.replaceAll("\"", "&quot;");
    returnString = returnString.replaceAll("'", "&apos;");
    return returnString;
  }

  /**
   * Predicate that decides if the given member should be considered part of an
   * API surface area. To make the most accurate decision, it searches for
   * signals on the member, all containing classes, and all containing packages.
   */
  public static class ApiPredicate implements Predicate<MemberInfo> {
    public boolean ignoreShown;
    public boolean ignoreRemoved;
    public boolean matchRemoved;

    /**
     * Set if the value of {@link MemberInfo#hasShowAnnotation()} should be
     * ignored. That is, this predicate will assume that all encountered members
     * match the "shown" requirement.
     * <p>
     * This is typically useful when generating "current.txt", when no
     * {@link Doclava#showAnnotations} have been defined.
     */
    public ApiPredicate setIgnoreShown(boolean ignoreShown) {
      this.ignoreShown = ignoreShown;
      return this;
    }

    /**
     * Set if the value of {@link MemberInfo#isRemoved()} should be ignored.
     * That is, this predicate will assume that all encountered members match
     * the "removed" requirement.
     * <p>
     * This is typically useful when generating "removed.txt", when it's okay to
     * reference both current and removed APIs.
     */
    public ApiPredicate setIgnoreRemoved(boolean ignoreRemoved) {
      this.ignoreRemoved = ignoreRemoved;
      return this;
    }

    /**
     * Set what the value of {@link MemberInfo#isRemoved()} must be equal to in
     * order for a member to match.
     * <p>
     * This is typically useful when generating "removed.txt", when you only
     * want to match members that have actually been removed.
     */
    public ApiPredicate setMatchRemoved(boolean matchRemoved) {
      this.matchRemoved = matchRemoved;
      return this;
    }

    private static PackageInfo containingPackage(PackageInfo pkg) {
      String name = pkg.name();
      final int lastDot = name.lastIndexOf('.');
      if (lastDot == -1) {
        return null;
      } else {
        name = name.substring(0, lastDot);
        return Converter.obtainPackage(name);
      }
    }

    @Override
    public boolean test(MemberInfo member) {
      boolean visible = member.isPublic() || member.isProtected();
      boolean hasShowAnnotation = member.hasShowAnnotation();
      boolean hidden = member.isHidden();
      boolean docOnly = member.isDocOnly();
      boolean removed = member.isRemoved();

      ClassInfo clazz = member.containingClass();
      if (clazz != null) {
        PackageInfo pkg = clazz.containingPackage();
        while (pkg != null) {
          hidden |= pkg.isHidden();
          docOnly |= pkg.isDocOnly();
          removed |= pkg.isRemoved();
          pkg = containingPackage(pkg);
        }
      }
      while (clazz != null) {
        visible &= clazz.isPublic() || clazz.isProtected();
        hasShowAnnotation |= clazz.hasShowAnnotation();
        hidden |= clazz.isHidden();
        docOnly |= clazz.isDocOnly();
        removed |= clazz.isRemoved();
        clazz = clazz.containingClass();
      }

      if (ignoreShown) {
        hasShowAnnotation = true;
      }
      if (ignoreRemoved) {
        removed = matchRemoved;
      }

      return visible && hasShowAnnotation && !hidden && !docOnly && (removed == matchRemoved);
    }
  }

  /**
   * Filter that will elide exact duplicate members that are already included
   * in another superclass/interfaces.
   */
  public static class ElidingPredicate implements Predicate<MemberInfo> {
    private final Predicate<MemberInfo> wrapped;

    public ElidingPredicate(Predicate<MemberInfo> wrapped) {
      this.wrapped = wrapped;
    }

    @Override
    public boolean test(MemberInfo member) {
      // This member should be included, but if it's an exact duplicate
      // override then we can elide it.
      if (member instanceof MethodInfo) {
        MethodInfo method = (MethodInfo) member;
        if (method.returnType() != null) {  // not a constructor
          String methodRaw = writeMethodApiWithoutDefault(method);
          return (method.findPredicateOverriddenMethod(new Predicate<MemberInfo>() {
            @Override
            public boolean test(MemberInfo test) {
              // We're looking for included and perfect signature
              return (wrapped.test(test)
                  && writeMethodApiWithoutDefault((MethodInfo) test).equals(methodRaw));
            }
          }) == null);
        }
      }
      return true;
    }
  }

  public static class FilterPredicate implements Predicate<MemberInfo> {
    private final Predicate<MemberInfo> wrapped;

    public FilterPredicate(Predicate<MemberInfo> wrapped) {
      this.wrapped = wrapped;
    }

    @Override
    public boolean test(MemberInfo member) {
      if (wrapped.test(member)) {
        return true;
      } else if (member instanceof MethodInfo) {
        MethodInfo method = (MethodInfo) member;
        return method.returnType() != null &&  // not a constructor
               method.findPredicateOverriddenMethod(wrapped) != null;
      } else {
        return false;
      }
    }
  }

  static void writeApi(PrintStream apiWriter, Map<PackageInfo, List<ClassInfo>> classesByPackage,
      Predicate<MemberInfo> filterEmit, Predicate<MemberInfo> filterReference) {
    for (PackageInfo pkg : classesByPackage.keySet().stream().sorted(PackageInfo.comparator)
        .collect(Collectors.toList())) {
      if (pkg.name().equals(PackageInfo.DEFAULT_PACKAGE)) continue;

      boolean hasWrittenPackageHead = false;
      for (ClassInfo clazz : classesByPackage.get(pkg).stream().sorted(ClassInfo.comparator)
          .collect(Collectors.toList())) {
        hasWrittenPackageHead = writeClassApi(apiWriter, clazz, filterEmit, filterReference,
            hasWrittenPackageHead);
      }

      if (hasWrittenPackageHead) {
        apiWriter.print("}\n\n");
      }
    }
  }

  static void writeDexApi(PrintStream apiWriter, Map<PackageInfo, List<ClassInfo>> classesByPackage,
      Predicate<MemberInfo> filterEmit) {
    for (PackageInfo pkg : classesByPackage.keySet().stream().sorted(PackageInfo.comparator)
        .collect(Collectors.toList())) {
      if (pkg.name().equals(PackageInfo.DEFAULT_PACKAGE)) continue;

      for (ClassInfo clazz : classesByPackage.get(pkg).stream().sorted(ClassInfo.comparator)
          .collect(Collectors.toList())) {
        writeClassDexApi(apiWriter, clazz, filterEmit);
      }
    }
  }

  /**
   * Write the removed members of the class to removed.txt
   */
  private static boolean writeClassApi(PrintStream apiWriter, ClassInfo cl,
      Predicate<MemberInfo> filterEmit, Predicate<MemberInfo> filterReference,
      boolean hasWrittenPackageHead) {

    List<MethodInfo> constructors = cl.getExhaustiveConstructors().stream().filter(filterEmit)
        .sorted(MethodInfo.comparator).collect(Collectors.toList());
    List<MethodInfo> methods = cl.getExhaustiveMethods().stream().filter(filterEmit)
        .sorted(MethodInfo.comparator).collect(Collectors.toList());
    List<FieldInfo> enums = cl.getExhaustiveEnumConstants().stream().filter(filterEmit)
        .sorted(FieldInfo.comparator).collect(Collectors.toList());
    List<FieldInfo> fields = cl.filteredFields(filterEmit).stream()
        .sorted(FieldInfo.comparator).collect(Collectors.toList());

    final boolean classEmpty = (constructors.isEmpty() && methods.isEmpty() && enums.isEmpty()
        && fields.isEmpty());
    final boolean emit;
    if (filterEmit.test(cl.asMemberInfo())) {
      emit = true;
    } else if (!classEmpty) {
      emit = filterReference.test(cl.asMemberInfo());
    } else {
      emit = false;
    }
    if (!emit) {
      return hasWrittenPackageHead;
    }

    // Look for Android @SystemApi exposed outside the normal SDK; we require
    // that they're protected with a system permission.
    if (Doclava.android && Doclava.showAnnotations.contains("android.annotation.SystemApi")) {
      boolean systemService = "android.content.pm.PackageManager".equals(cl.qualifiedName());
      for (AnnotationInstanceInfo a : cl.annotations()) {
        if (a.type().qualifiedNameMatches("android", "annotation.SystemService")) {
          systemService = true;
        }
      }
      if (systemService) {
        for (MethodInfo mi : methods) {
          checkSystemPermissions(mi);
        }
      }
    }

    for (MethodInfo method : methods) {
      checkHiddenTypes(method, filterReference);
    }
    for (FieldInfo field : fields) {
      checkHiddenTypes(field, filterReference);
    }

    if (!hasWrittenPackageHead) {
      hasWrittenPackageHead = true;
      apiWriter.print("package ");
      apiWriter.print(cl.containingPackage().qualifiedName());
      apiWriter.print(" {\n\n");
    }

    apiWriter.print("  ");
    apiWriter.print(cl.scope());
    if (cl.isStatic()) {
      apiWriter.print(" static");
    }
    if (cl.isFinal()) {
      apiWriter.print(" final");
    }
    if (cl.isAbstract()) {
      apiWriter.print(" abstract");
    }
    if (cl.isDeprecated()) {
      apiWriter.print(" deprecated");
    }
    apiWriter.print(" ");
    apiWriter.print(cl.isInterface() ? "interface" : "class");
    apiWriter.print(" ");
    apiWriter.print(cl.name());
    if (cl.hasTypeParameters()) {
      apiWriter.print(TypeInfo.typeArgumentsName(cl.asTypeInfo().typeArguments(),
          new HashSet<String>()));
    }

    if (!cl.isInterface()
        && !"java.lang.Object".equals(cl.qualifiedName())) {
      final ClassInfo superclass = cl.filteredSuperclass(filterReference);
      if (superclass != null && !"java.lang.Object".equals(superclass.qualifiedName())) {
        apiWriter.print(" extends ");
        apiWriter.print(superclass.qualifiedName());
      }
    }

    List<ClassInfo> interfaces = cl.filteredInterfaces(filterReference).stream()
        .sorted(ClassInfo.comparator).collect(Collectors.toList());
    boolean first = true;
    for (ClassInfo iface : interfaces) {
      if (first) {
        apiWriter.print(" implements");
        first = false;
      }
      apiWriter.print(" ");
      apiWriter.print(iface.qualifiedName());
    }

    apiWriter.print(" {\n");

    for (MethodInfo mi : constructors) {
      writeConstructorApi(apiWriter, mi);
    }
    for (MethodInfo mi : methods) {
      writeMethodApi(apiWriter, mi);
    }
    for (FieldInfo fi : enums) {
      writeFieldApi(apiWriter, fi, "enum_constant");
    }
    for (FieldInfo fi : fields) {
      writeFieldApi(apiWriter, fi, "field");
    }

    apiWriter.print("  }\n\n");
    return hasWrittenPackageHead;
  }

  private static void writeClassDexApi(PrintStream apiWriter, ClassInfo cl,
      Predicate<MemberInfo> filterEmit) {
    if (filterEmit.test(cl.asMemberInfo())) {
      apiWriter.print(toSlashFormat(cl.qualifiedName()));
      apiWriter.print("\n");
    }

    List<MethodInfo> constructors = cl.getExhaustiveConstructors().stream().filter(filterEmit)
        .sorted(MethodInfo.comparator).collect(Collectors.toList());
    List<MethodInfo> methods = cl.getExhaustiveMethods().stream().filter(filterEmit)
        .sorted(MethodInfo.comparator).collect(Collectors.toList());
    List<FieldInfo> enums = cl.getExhaustiveEnumConstants().stream().filter(filterEmit)
        .sorted(FieldInfo.comparator).collect(Collectors.toList());
    List<FieldInfo> fields = cl.getExhaustiveFields().stream().filter(filterEmit)
        .sorted(FieldInfo.comparator).collect(Collectors.toList());

    for (MethodInfo mi : constructors) {
      writeMethodDexApi(apiWriter, cl, mi);
    }
    for (MethodInfo mi : methods) {
      writeMethodDexApi(apiWriter, cl, mi);
    }
    for (FieldInfo fi : enums) {
      writeFieldDexApi(apiWriter, cl, fi);
    }
    for (FieldInfo fi : fields) {
      writeFieldDexApi(apiWriter, cl, fi);
    }
  }

  private static void checkSystemPermissions(MethodInfo mi) {
    boolean hasAnnotation = false;
    for (AnnotationInstanceInfo a : mi.annotations()) {
      if (a.type().qualifiedNameMatches("android", "annotation.RequiresPermission")) {
        hasAnnotation = true;
        for (AnnotationValueInfo val : a.elementValues()) {
          ArrayList<AnnotationValueInfo> values = new ArrayList<>();
          boolean any = false;
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
          if (values.isEmpty()) continue;

          ArrayList<String> system = new ArrayList<>();
          ArrayList<String> nonSystem = new ArrayList<>();
          for (AnnotationValueInfo value : values) {
            final String perm = String.valueOf(value.value());
            final String level = Doclava.manifestPermissions.getOrDefault(perm, null);
            if (level == null) {
              Errors.error(Errors.REMOVED_FIELD, mi.position(),
                  "Permission '" + perm + "' is not defined by AndroidManifest.xml.");
              continue;
            }
            if (level.contains("normal") || level.contains("dangerous")
                || level.contains("ephemeral")) {
              nonSystem.add(perm);
            } else {
              system.add(perm);
            }
          }

          if (system.isEmpty() && nonSystem.isEmpty()) {
            hasAnnotation = false;
          } else if ((any && !nonSystem.isEmpty()) || (!any && system.isEmpty())) {
            Errors.error(Errors.REQUIRES_PERMISSION, mi, "Method '" + mi.name()
                + "' must be protected with a system permission; it currently"
                + " allows non-system callers holding " + nonSystem.toString());
          }
        }
      }
    }
    if (!hasAnnotation) {
      Errors.error(Errors.REQUIRES_PERMISSION, mi, "Method '" + mi.name()
        + "' must be protected with a system permission.");
    }
  }

  private static void checkHiddenTypes(MethodInfo method, Predicate<MemberInfo> filterReference) {
    checkHiddenTypes(method.returnType(), method, filterReference);
    List<ParameterInfo> params = method.parameters();
    if (params != null) {
      for (ParameterInfo param : params) {
        checkHiddenTypes(param.type(), method, filterReference);
      }
    }
  }

  private static void checkHiddenTypes(FieldInfo field, Predicate<MemberInfo> filterReference) {
    checkHiddenTypes(field.type(), field, filterReference);
  }

  private static void checkHiddenTypes(TypeInfo type, MemberInfo member,
      Predicate<MemberInfo> filterReference) {
    if (type == null || type.isPrimitive()) {
      return;
    }

    ClassInfo clazz = type.asClassInfo();
    if (clazz == null || !filterReference.test(clazz.asMemberInfo())) {
      Errors.error(Errors.HIDDEN_TYPE_PARAMETER, member.position(),
          "Member " + member + " references hidden type " + type.qualifiedTypeName() + ".");
    }

    List<TypeInfo> args = type.typeArguments();
    if (args != null) {
      for (TypeInfo arg : args) {
        checkHiddenTypes(arg, member, filterReference);
      }
    }
  }

  static void writeConstructorApi(PrintStream apiWriter, MethodInfo mi) {
    apiWriter.print("    ctor ");
    apiWriter.print(mi.scope());
    if (mi.isDeprecated()) {
      apiWriter.print(" deprecated");
    }
    apiWriter.print(" ");
    apiWriter.print(mi.name());

    writeParametersApi(apiWriter, mi, mi.parameters());
    writeThrowsApi(apiWriter, mi.thrownExceptions());
    apiWriter.print(";\n");
  }

  static String writeMethodApiWithoutDefault(MethodInfo mi) {
    final ByteArrayOutputStream out = new ByteArrayOutputStream();
    writeMethodApi(new PrintStream(out), mi, false);
    return out.toString();
  }

  static void writeMethodApi(PrintStream apiWriter, MethodInfo mi) {
    writeMethodApi(apiWriter, mi, true);
  }

  static void writeMethodApi(PrintStream apiWriter, MethodInfo mi, boolean withDefault) {
    apiWriter.print("    method ");
    apiWriter.print(mi.scope());
    if (mi.isDefault() && withDefault) {
      apiWriter.print(" default");
    }
    if (mi.isStatic()) {
      apiWriter.print(" static");
    }
    if (mi.isFinal()) {
      apiWriter.print(" final");
    }
    if (mi.isAbstract()) {
      apiWriter.print(" abstract");
    }
    if (mi.isDeprecated()) {
      apiWriter.print(" deprecated");
    }
    if (mi.isSynchronized()) {
      apiWriter.print(" synchronized");
    }
    if (mi.hasTypeParameters()) {
      apiWriter.print(" " + mi.typeArgumentsName(new HashSet<String>()));
    }
    apiWriter.print(" ");
    if (mi.returnType() == null) {
      apiWriter.print("void");
    } else {
      apiWriter.print(fullParameterTypeName(mi, mi.returnType(), false));
    }
    apiWriter.print(" ");
    apiWriter.print(mi.name());

    writeParametersApi(apiWriter, mi, mi.parameters());
    writeThrowsApi(apiWriter, mi.thrownExceptions());

    apiWriter.print(";\n");
  }

  static void writeMethodDexApi(PrintStream apiWriter, ClassInfo cl, MethodInfo mi) {
    apiWriter.print(toSlashFormat(cl.qualifiedName()));
    apiWriter.print("->");
    if (mi.returnType() == null) {
      apiWriter.print("<init>");
    } else {
      apiWriter.print(mi.name());
    }
    writeParametersDexApi(apiWriter, mi, mi.parameters());
    if (mi.returnType() == null) {  // constructor
      apiWriter.print("V");
    } else {
      apiWriter.print(toSlashFormat(mi.returnType().dexName()));
    }
    apiWriter.print("\n");
  }

  static void writeParametersApi(PrintStream apiWriter, MethodInfo method,
      ArrayList<ParameterInfo> params) {
    apiWriter.print("(");

    for (ParameterInfo pi : params) {
      if (pi != params.get(0)) {
        apiWriter.print(", ");
      }
      apiWriter.print(fullParameterTypeName(method, pi.type(), pi == params.get(params.size()-1)));
      // turn on to write the names too
      if (false) {
        apiWriter.print(" ");
        apiWriter.print(pi.name());
      }
    }

    apiWriter.print(")");
  }

  static void writeParametersDexApi(PrintStream apiWriter, MethodInfo method,
      ArrayList<ParameterInfo> params) {
    apiWriter.print("(");
    for (ParameterInfo pi : params) {
      String typeName = pi.type().dexName();
      if (method.isVarArgs() && pi == params.get(params.size() - 1)) {
        typeName += "[]";
      }
      apiWriter.print(toSlashFormat(typeName));
    }
    apiWriter.print(")");
  }

  static void writeThrowsApi(PrintStream apiWriter, ArrayList<ClassInfo> exceptions) {
    // write in a canonical order
    exceptions = (ArrayList<ClassInfo>) exceptions.clone();
    Collections.sort(exceptions, ClassInfo.comparator);
    //final int N = exceptions.length;
    boolean first = true;
    for (ClassInfo ex : exceptions) {
      // Turn this off, b/c we need to regenrate the old xml files.
      if (true || !"java.lang.RuntimeException".equals(ex.qualifiedName())
          && !ex.isDerivedFrom("java.lang.RuntimeException")) {
        if (first) {
          apiWriter.print(" throws ");
          first = false;
        } else {
          apiWriter.print(", ");
        }
        apiWriter.print(ex.qualifiedName());
      }
    }
  }

  static void writeFieldApi(PrintStream apiWriter, FieldInfo fi, String label) {
    apiWriter.print("    ");
    apiWriter.print(label);
    apiWriter.print(" ");
    apiWriter.print(fi.scope());
    if (fi.isStatic()) {
      apiWriter.print(" static");
    }
    if (fi.isFinal()) {
      apiWriter.print(" final");
    }
    if (fi.isDeprecated()) {
      apiWriter.print(" deprecated");
    }
    if (fi.isTransient()) {
      apiWriter.print(" transient");
    }
    if (fi.isVolatile()) {
      apiWriter.print(" volatile");
    }

    apiWriter.print(" ");
    apiWriter.print(fi.type().fullName(fi.typeVariables()));

    apiWriter.print(" ");
    apiWriter.print(fi.name());

    Object val = null;
    if (fi.isConstant() && fieldIsInitialized(fi)) {
      apiWriter.print(" = ");
      apiWriter.print(fi.constantLiteralValue());
      val = fi.constantValue();
    }

    apiWriter.print(";");

    if (val != null) {
      if (val instanceof Integer && "char".equals(fi.type().qualifiedTypeName())) {
        apiWriter.format(" // 0x%04x '%s'", val,
            FieldInfo.javaEscapeString("" + ((char)((Integer)val).intValue())));
      } else if (val instanceof Byte || val instanceof Short || val instanceof Integer) {
        apiWriter.format(" // 0x%x", val);
      } else if (val instanceof Long) {
        apiWriter.format(" // 0x%xL", val);
      }
    }

    apiWriter.print("\n");
  }

  static void writeFieldDexApi(PrintStream apiWriter, ClassInfo cl, FieldInfo fi) {
    apiWriter.print(toSlashFormat(cl.qualifiedName()));
    apiWriter.print("->");
    apiWriter.print(fi.name());
    apiWriter.print(":");
    apiWriter.print(toSlashFormat(fi.type().dexName()));
    apiWriter.print("\n");
  }

  static void writeKeepList(PrintStream keepListWriter,
      HashMap<PackageInfo, List<ClassInfo>> allClasses, HashSet<ClassInfo> notStrippable) {
    // extract the set of packages, sort them by name, and write them out in that order
    Set<PackageInfo> allClassKeys = allClasses.keySet();
    PackageInfo[] allPackages = allClassKeys.toArray(new PackageInfo[allClassKeys.size()]);
    Arrays.sort(allPackages, PackageInfo.comparator);

    for (PackageInfo pack : allPackages) {
      writePackageKeepList(keepListWriter, pack, allClasses.get(pack), notStrippable);
    }
  }

  static void writePackageKeepList(PrintStream keepListWriter, PackageInfo pack,
      Collection<ClassInfo> classList, HashSet<ClassInfo> notStrippable) {
    // Work around the bogus "Array" class we invent for
    // Arrays.copyOf's Class<? extends T[]> newType parameter. (http://b/2715505)
    if (pack.name().equals(PackageInfo.DEFAULT_PACKAGE)) {
      return;
    }

    ClassInfo[] classes = classList.toArray(new ClassInfo[classList.size()]);
    Arrays.sort(classes, ClassInfo.comparator);
    for (ClassInfo cl : classes) {
      writeClassKeepList(keepListWriter, cl, notStrippable);
    }
  }

  static void writeClassKeepList(PrintStream keepListWriter, ClassInfo cl,
      HashSet<ClassInfo> notStrippable) {
    keepListWriter.print("-keep class ");
    keepListWriter.print(to$Class(cl.qualifiedName()));

    keepListWriter.print(" {\n");

    ArrayList<MethodInfo> constructors = cl.constructors();
    Collections.sort(constructors, MethodInfo.comparator);
    for (MethodInfo mi : constructors) {
      writeConstructorKeepList(keepListWriter, mi);
    }

    keepListWriter.print("\n");

    ArrayList<MethodInfo> methods = cl.allSelfMethods();
    Collections.sort(methods, MethodInfo.comparator);
    for (MethodInfo mi : methods) {
      // allSelfMethods is the non-hidden and visible methods. See Doclava.checkLevel.
      writeMethodKeepList(keepListWriter, mi);
    }

    keepListWriter.print("\n");

    ArrayList<FieldInfo> enums = cl.enumConstants();
    Collections.sort(enums, FieldInfo.comparator);
    for (FieldInfo fi : enums) {
      writeFieldKeepList(keepListWriter, fi);
    }

    keepListWriter.print("\n");

    ArrayList<FieldInfo> fields = cl.selfFields();
    Collections.sort(fields, FieldInfo.comparator);
    for (FieldInfo fi : fields) {
      writeFieldKeepList(keepListWriter, fi);
    }

    keepListWriter.print("}\n\n");
  }

  static void writeConstructorKeepList(PrintStream keepListWriter, MethodInfo mi) {
    keepListWriter.print("    ");
    keepListWriter.print("<init>");

    writeParametersKeepList(keepListWriter, mi, mi.parameters());
    keepListWriter.print(";\n");
  }

  static void writeMethodKeepList(PrintStream keepListWriter, MethodInfo mi) {
    keepListWriter.print("    ");
    keepListWriter.print(mi.scope());
    if (mi.isStatic()) {
      keepListWriter.print(" static");
    }
    if (mi.isAbstract()) {
      keepListWriter.print(" abstract");
    }
    if (mi.isSynchronized()) {
      keepListWriter.print(" synchronized");
    }
    keepListWriter.print(" ");
    if (mi.returnType() == null) {
      keepListWriter.print("void");
    } else {
      keepListWriter.print(getCleanTypeName(mi.returnType()));
    }
    keepListWriter.print(" ");
    keepListWriter.print(mi.name());

    writeParametersKeepList(keepListWriter, mi, mi.parameters());

    keepListWriter.print(";\n");
  }

  static void writeParametersKeepList(PrintStream keepListWriter, MethodInfo method,
      ArrayList<ParameterInfo> params) {
    keepListWriter.print("(");

    for (ParameterInfo pi : params) {
      if (pi != params.get(0)) {
        keepListWriter.print(", ");
      }
      keepListWriter.print(getCleanTypeName(pi.type()));
    }

    keepListWriter.print(")");
  }

  static void writeFieldKeepList(PrintStream keepListWriter, FieldInfo fi) {
    keepListWriter.print("    ");
    keepListWriter.print(fi.scope());
    if (fi.isStatic()) {
      keepListWriter.print(" static");
    }
    if (fi.isTransient()) {
      keepListWriter.print(" transient");
    }
    if (fi.isVolatile()) {
      keepListWriter.print(" volatile");
    }

    keepListWriter.print(" ");
    keepListWriter.print(getCleanTypeName(fi.type()));

    keepListWriter.print(" ");
    keepListWriter.print(fi.name());

    keepListWriter.print(";\n");
  }

  static String fullParameterTypeName(MethodInfo method, TypeInfo type, boolean isLast) {
    String fullTypeName = type.fullName(method.typeVariables());
    if (isLast && method.isVarArgs()) {
      // TODO: note that this does not attempt to handle hypothetical
      // vararg methods whose last parameter is a list of arrays, e.g.
      // "Object[]...".
      fullTypeName = type.fullNameNoDimension(method.typeVariables()) + "...";
    }
    return fullTypeName;
  }

  static String to$Class(String name) {
    int pos = 0;
    while ((pos = name.indexOf('.', pos)) > 0) {
      String n = name.substring(0, pos);
      if (Converter.obtainClass(n) != null) {
        return n + (name.substring(pos).replace('.', '$'));
      }
      pos = pos + 1;
    }
    return name;
  }

  static String toSlashFormat(String name) {
    String dimension = "";
    while (name.endsWith("[]")) {
      dimension += "[";
      name = name.substring(0, name.length() - 2);
    }

    final String base;
    if (name.equals("void")) {
      base = "V";
    } else if (name.equals("byte")) {
      base = "B";
    } else if (name.equals("boolean")) {
      base = "Z";
    } else if (name.equals("char")) {
      base = "C";
    } else if (name.equals("short")) {
      base = "S";
    } else if (name.equals("int")) {
      base = "I";
    } else if (name.equals("long")) {
      base = "J";
    } else if (name.equals("float")) {
      base = "F";
    } else if (name.equals("double")) {
      base = "D";
    } else {
      base = "L" + to$Class(name).replace(".", "/") + ";";
    }

    return dimension + base;
  }

  static String getCleanTypeName(TypeInfo t) {
      return t.isPrimitive() ? t.simpleTypeName() + t.dimension() :
              to$Class(t.asClassInfo().qualifiedName() + t.dimension());
  }
}
