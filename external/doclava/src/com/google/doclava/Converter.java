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

import com.google.doclava.apicheck.ApiInfo;
import com.sun.javadoc.AnnotationDesc;
import com.sun.javadoc.AnnotationTypeDoc;
import com.sun.javadoc.AnnotationTypeElementDoc;
import com.sun.javadoc.AnnotationValue;
import com.sun.javadoc.ClassDoc;
import com.sun.javadoc.ConstructorDoc;
import com.sun.javadoc.ExecutableMemberDoc;
import com.sun.javadoc.FieldDoc;
import com.sun.javadoc.MemberDoc;
import com.sun.javadoc.MethodDoc;
import com.sun.javadoc.PackageDoc;
import com.sun.javadoc.ParamTag;
import com.sun.javadoc.Parameter;
import com.sun.javadoc.RootDoc;
import com.sun.javadoc.SeeTag;
import com.sun.javadoc.SourcePosition;
import com.sun.javadoc.Tag;
import com.sun.javadoc.ThrowsTag;
import com.sun.javadoc.Type;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

public class Converter {
  private static RootDoc root;
  private static List<ApiInfo> apis;

  public static void makeInfo(RootDoc r) {
    root = r;
    apis = new ArrayList<>();

    // create the objects
    ClassDoc[] classes = getClasses(r);
    for (ClassDoc c : classes) {
      Converter.obtainClass(c);
    }

    ArrayList<ClassInfo> classesNeedingInit2 = new ArrayList<ClassInfo>();

    int i;
    // fill in the fields that reference other classes
    while (mClassesNeedingInit.size() > 0) {
      i = mClassesNeedingInit.size() - 1;
      ClassNeedingInit clni = mClassesNeedingInit.get(i);
      mClassesNeedingInit.remove(i);

      initClass(clni.c, clni.cl);
      classesNeedingInit2.add(clni.cl);
    }
    mClassesNeedingInit = null;
    for (ClassInfo cl : classesNeedingInit2) {
      cl.init2();
    }

    finishAnnotationValueInit();

    // fill in the "root" stuff
    mRootClasses = Converter.convertClasses(classes);
  }

  /**
   * Adds additional APIs to be available from calls to obtain() methods.
   *
   * @param api the APIs to add, must be non-{@code null}
   */
  public static void addApiInfo(ApiInfo api) {
    apis.add(api);
  }

  private static ClassDoc[] getClasses(RootDoc r) {
    ClassDoc[] classDocs = r.classes();
    ArrayList<ClassDoc> filtered = new ArrayList<ClassDoc>(classDocs.length);
    for (ClassDoc c : classDocs) {
      if (c.position() != null) {
        // Work around a javadoc bug in Java 7: We sometimes spuriously
        // receive duplicate top level ClassDocs with null positions and no type
        // information. Ignore them, since every ClassDoc must have a non null
        // position.

        filtered.add(c);
      }
    }

    ClassDoc[] filteredArray = new ClassDoc[filtered.size()];
    filtered.toArray(filteredArray);
    return filteredArray;
  }

  private static ClassInfo[] mRootClasses;

  public static Collection<ClassInfo> rootClasses() {
    return Arrays.asList(mRootClasses);
  }

  public static Collection<ClassInfo> allClasses() {
    return (Collection<ClassInfo>) mClasses.all();
  }

  private static final MethodDoc[] EMPTY_METHOD_DOC = new MethodDoc[0];

  private static void initClass(ClassDoc c, ClassInfo cl) {
    MethodDoc[] annotationElements;
    if (c instanceof AnnotationTypeDoc) {
      annotationElements = ((AnnotationTypeDoc) c).elements();
    } else {
      annotationElements = EMPTY_METHOD_DOC;
    }
    cl.init(Converter.obtainType(c),
            new ArrayList<ClassInfo>(Arrays.asList(Converter.convertClasses(c.interfaces()))),
            new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(c.interfaceTypes()))),
            new ArrayList<ClassInfo>(Arrays.asList(Converter.convertClasses(c.innerClasses()))),
            new ArrayList<MethodInfo>(Arrays.asList(
                    Converter.convertMethods(c.constructors(false)))),
            new ArrayList<MethodInfo>(Arrays.asList(Converter.convertMethods(c.methods(false)))),
            new ArrayList<MethodInfo>(Arrays.asList(Converter.convertMethods(annotationElements))),
            new ArrayList<FieldInfo>(Arrays.asList(Converter.convertFields(c.fields(false)))),
            new ArrayList<FieldInfo>(Arrays.asList(Converter.convertFields(c.enumConstants()))),
            Converter.obtainPackage(c.containingPackage()),
            Converter.obtainClass(c.containingClass()),
            Converter.obtainClass(c.superclass()), Converter.obtainType(c.superclassType()),
            new ArrayList<AnnotationInstanceInfo>(Arrays.asList(
                    Converter.convertAnnotationInstances(c.annotations()))));

    cl.setHiddenMethods(
            new ArrayList<MethodInfo>(Arrays.asList(Converter.getHiddenMethods(c.methods(false)))));
    cl.setRemovedMethods(
            new ArrayList<MethodInfo>(Arrays.asList(Converter.getRemovedMethods(c.methods(false)))));

    cl.setExhaustiveConstructors(
        new ArrayList<MethodInfo>(Converter.convertAllMethods(c.constructors(false))));
    cl.setExhaustiveMethods(
        new ArrayList<MethodInfo>(Converter.convertAllMethods(c.methods(false))));
    cl.setExhaustiveEnumConstants(
        new ArrayList<FieldInfo>(Converter.convertAllFields(c.enumConstants())));
    cl.setExhaustiveFields(
        new ArrayList<FieldInfo>(Converter.convertAllFields(c.fields(false))));

    cl.setNonWrittenConstructors(
            new ArrayList<MethodInfo>(Arrays.asList(Converter.convertNonWrittenConstructors(
                    c.constructors(false)))));
    cl.init3(
            new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(c.typeParameters()))),
            new ArrayList<ClassInfo>(Arrays.asList(
                    Converter.convertClasses(c.innerClasses(false)))));
  }

  /**
   * Obtains a {@link ClassInfo} describing the specified class. If the class
   * was not specified on the source path, this method will attempt to locate
   * the class within the list of federated APIs.
   *
   * @param className the fully-qualified class name to search for
   * @return info for the specified class, or {@code null} if not available
   * @see #addApiInfo(ApiInfo)
   */
  public static ClassInfo obtainClass(String className) {
    ClassInfo result = Converter.obtainClass(root.classNamed(className));
    if (result != null) {
      return result;
    }

    for (ApiInfo api : apis) {
      result = api.findClass(className);
      if (result != null) {
        return result;
      }
    }

    return null;
  }

  /**
   * Obtains a {@link PackageInfo} describing the specified package. If the
   * package was not specified on the source path, this method will attempt to
   * locate the package within the list of federated APIs.
   *
   * @param packageName the fully-qualified package name to search for
   * @return info for the specified package, or {@code null} if not available
   * @see #addApiInfo(ApiInfo)
   */
  public static PackageInfo obtainPackage(String packageName) {
    PackageInfo result = Converter.obtainPackage(root.packageNamed(packageName));
    if (result != null) {
      return result;
    }

    for (ApiInfo api : apis) {
      result = api.getPackages().get(packageName);
      if (result != null) {
        return result;
      }
    }

    return null;
  }

  private static TagInfo convertTag(Tag tag) {
    return new TextTagInfo(tag.name(), tag.kind(), tag.text(),
            Converter.convertSourcePosition(tag.position()));
  }

  private static ThrowsTagInfo convertThrowsTag(ThrowsTag tag, ContainerInfo base) {
    return new ThrowsTagInfo(tag.name(), tag.text(), tag.kind(), Converter.obtainClass(tag
        .exception()), tag.exceptionComment(), base, Converter
        .convertSourcePosition(tag.position()));
  }

  private static ParamTagInfo convertParamTag(ParamTag tag, ContainerInfo base) {
    return new ParamTagInfo(tag.name(), tag.kind(), tag.text(), tag.isTypeParameter(), tag
        .parameterComment(), tag.parameterName(), base, Converter.convertSourcePosition(tag
        .position()));
  }

  private static SeeTagInfo convertSeeTag(SeeTag tag, ContainerInfo base) {
    return new SeeTagInfo(tag.name(), tag.kind(), tag.text(), base, Converter
        .convertSourcePosition(tag.position()));
  }

  private static SourcePositionInfo convertSourcePosition(SourcePosition sp) {
    if (sp == null) {
      return null;
    }
    return new SourcePositionInfo(sp.file().toString(), sp.line(), sp.column());
  }

  public static TagInfo[] convertTags(Tag[] tags, ContainerInfo base) {
    int len = tags.length;
    TagInfo[] out = TagInfo.getArray(len);
    for (int i = 0; i < len; i++) {
      Tag t = tags[i];
      /*
       * System.out.println("Tag name='" + t.name() + "' kind='" + t.kind() + "'");
       */
      if (t instanceof SeeTag) {
        out[i] = Converter.convertSeeTag((SeeTag) t, base);
      } else if (t instanceof ThrowsTag) {
        out[i] = Converter.convertThrowsTag((ThrowsTag) t, base);
      } else if (t instanceof ParamTag) {
        out[i] = Converter.convertParamTag((ParamTag) t, base);
      } else {
        out[i] = Converter.convertTag(t);
      }
    }
    return out;
  }

  public static ClassInfo[] convertClasses(ClassDoc[] classes) {
    if (classes == null) return null;
    int N = classes.length;
    ClassInfo[] result = new ClassInfo[N];
    for (int i = 0; i < N; i++) {
      result[i] = Converter.obtainClass(classes[i]);
    }
    return result;
  }

  private static ParameterInfo convertParameter(Parameter p, SourcePosition pos, boolean isVarArg) {
    if (p == null) return null;
    ParameterInfo pi =
        new ParameterInfo(p.name(), p.typeName(), Converter.obtainType(p.type()), isVarArg,
          Converter.convertSourcePosition(pos),
          Arrays.asList(Converter.convertAnnotationInstances(p.annotations())));
    return pi;
  }

  private static ParameterInfo[] convertParameters(Parameter[] p, ExecutableMemberDoc m) {
    SourcePosition pos = m.position();
    int len = p.length;
    ParameterInfo[] q = new ParameterInfo[len];
    for (int i = 0; i < len; i++) {
      boolean isVarArg = (m.isVarArgs() && i == len - 1);
      q[i] = Converter.convertParameter(p[i], pos, isVarArg);
    }
    return q;
  }

  private static TypeInfo[] convertTypes(Type[] p) {
    if (p == null) return null;
    int len = p.length;
    TypeInfo[] q = new TypeInfo[len];
    for (int i = 0; i < len; i++) {
      q[i] = Converter.obtainType(p[i]);
    }
    return q;
  }

  private Converter() {}

  private static class ClassNeedingInit {
    ClassNeedingInit(ClassDoc c, ClassInfo cl) {
      this.c = c;
      this.cl = cl;
    }

    ClassDoc c;
    ClassInfo cl;
  }

  private static ArrayList<ClassNeedingInit> mClassesNeedingInit =
      new ArrayList<ClassNeedingInit>();

  static ClassInfo obtainClass(ClassDoc o) {
    return (ClassInfo) mClasses.obtain(o);
  }

  private static Cache mClasses = new Cache() {
    @Override
    protected Object make(Object o) {
      ClassDoc c = (ClassDoc) o;
      ClassInfo cl =
          new ClassInfo(c, c.getRawCommentText(), Converter.convertSourcePosition(c.position()), c
              .isPublic(), c.isProtected(), c.isPackagePrivate(), c.isPrivate(), c.isStatic(), c
              .isInterface(), c.isAbstract(), c.isOrdinaryClass(), c.isException(), c.isError(), c
              .isEnum(), (c instanceof AnnotationTypeDoc), c.isFinal(), c.isIncluded(), c.name(), c
              .qualifiedName(), c.qualifiedTypeName(), c.isPrimitive());
      if (mClassesNeedingInit != null) {
        mClassesNeedingInit.add(new ClassNeedingInit(c, cl));
      }
      return cl;
    }

    @Override
    protected void made(Object o, Object r) {
      if (mClassesNeedingInit == null) {
        initClass((ClassDoc) o, (ClassInfo) r);
        ((ClassInfo) r).init2();
      }
    }

    @Override
    Collection<?> all() {
      return mCache.values();
    }
  };

  private static MethodInfo[] getHiddenMethods(MethodDoc[] methods) {
    if (methods == null) return null;
    ArrayList<MethodInfo> hiddenMethods = new ArrayList<MethodInfo>();
    for (MethodDoc method : methods) {
      MethodInfo methodInfo = Converter.obtainMethod(method);
      if (methodInfo.isHidden()) {
        hiddenMethods.add(methodInfo);
      }
    }

    return hiddenMethods.toArray(new MethodInfo[hiddenMethods.size()]);
  }

  // Gets the removed methods regardless of access levels
  private static MethodInfo[] getRemovedMethods(MethodDoc[] methods) {
    if (methods == null) return null;
    ArrayList<MethodInfo> removedMethods = new ArrayList<MethodInfo>();
    for (MethodDoc method : methods) {
      MethodInfo methodInfo = Converter.obtainMethod(method);
      if (methodInfo.isRemoved()) {
        removedMethods.add(methodInfo);
      }
    }

    return removedMethods.toArray(new MethodInfo[removedMethods.size()]);
  }

  /**
   * Converts FieldDoc[] into List<FieldInfo>. No filtering is done.
   */
  private static List<FieldInfo> convertAllFields(FieldDoc[] fields) {
    if (fields == null) return null;
    List<FieldInfo> allFields = new ArrayList<FieldInfo>();

    for (FieldDoc field : fields) {
      FieldInfo fieldInfo = Converter.obtainField(field);
      allFields.add(fieldInfo);
    }

    return allFields;
  }

  /**
   * Converts ExecutableMemberDoc[] into List<MethodInfo>. No filtering is done.
   */
  private static List<MethodInfo> convertAllMethods(ExecutableMemberDoc[] methods) {
    if (methods == null) return null;
    List<MethodInfo> allMethods = new ArrayList<MethodInfo>();
    for (ExecutableMemberDoc method : methods) {
      MethodInfo methodInfo = Converter.obtainMethod(method);
      allMethods.add(methodInfo);
    }
    return allMethods;
  }

  /**
   * Convert MethodDoc[] or ConstructorDoc[] into MethodInfo[].
   * Also filters according to the -private, -public option,
   * because the filtering doesn't seem to be working in the ClassDoc.constructors(boolean) call.
   */
  private static MethodInfo[] convertMethods(ExecutableMemberDoc[] methods) {
    if (methods == null) return null;
    List<MethodInfo> filteredMethods = new ArrayList<MethodInfo>();
    for (ExecutableMemberDoc method : methods) {
      MethodInfo methodInfo = Converter.obtainMethod(method);
      if (methodInfo.checkLevel()) {
        filteredMethods.add(methodInfo);
      }
    }

    return filteredMethods.toArray(new MethodInfo[filteredMethods.size()]);
  }

  private static MethodInfo[] convertNonWrittenConstructors(ConstructorDoc[] methods) {
    if (methods == null) return null;
    ArrayList<MethodInfo> ctors = new ArrayList<MethodInfo>();
    for (ConstructorDoc method : methods) {
      MethodInfo methodInfo = Converter.obtainMethod(method);
      if (!methodInfo.checkLevel()) {
        ctors.add(methodInfo);
      }
    }

    return ctors.toArray(new MethodInfo[ctors.size()]);
  }

  private static <E extends ExecutableMemberDoc> MethodInfo obtainMethod(E o) {
    return (MethodInfo) mMethods.obtain(o);
  }

  private static Cache mMethods = new Cache() {
    @Override
    protected Object make(Object o) {
      if (o instanceof AnnotationTypeElementDoc) {
        AnnotationTypeElementDoc m = (AnnotationTypeElementDoc) o;
        MethodInfo result =
            new MethodInfo(m.getRawCommentText(),
                    new ArrayList<TypeInfo>(Arrays.asList(
                            Converter.convertTypes(m.typeParameters()))),
                    m.name(), m.signature(), Converter.obtainClass(m.containingClass()),
                    Converter.obtainClass(m.containingClass()), m.isPublic(), m.isProtected(), m
                    .isPackagePrivate(), m.isPrivate(), m.isFinal(), m.isStatic(), m.isSynthetic(),
                    m.isAbstract(), m.isSynchronized(), m.isNative(), m.isDefault(), true,
                    "annotationElement", m.flatSignature(),
                    Converter.obtainMethod(m.overriddenMethod()),
                    Converter.obtainType(m.returnType()),
                    new ArrayList<ParameterInfo>(Arrays.asList(
                            Converter.convertParameters(m.parameters(), m))),
                    new ArrayList<ClassInfo>(Arrays.asList(Converter.convertClasses(
                            m.thrownExceptions()))), Converter.convertSourcePosition(m.position()),
                    new ArrayList<AnnotationInstanceInfo>(Arrays.asList(
                            Converter.convertAnnotationInstances(m.annotations()))));
        result.setVarargs(m.isVarArgs());
        result.init(Converter.obtainAnnotationValue(m.defaultValue(), result));
        return result;
      } else if (o instanceof MethodDoc) {
        MethodDoc m = (MethodDoc) o;
        final ClassInfo containingClass = Converter.obtainClass(m.containingClass());

        // The containing class is final, so it is implied that every method is final as well.
        // No need to apply 'final' to each method.
        final boolean isMethodFinal = m.isFinal() && !containingClass.isFinal();
        MethodInfo result =
            new MethodInfo(m.getRawCommentText(),
                    new ArrayList<TypeInfo>(Arrays.asList(
                            Converter.convertTypes(m.typeParameters()))), m.name(), m.signature(),
                    containingClass, containingClass, m.isPublic(), m.isProtected(),
                    m.isPackagePrivate(), m.isPrivate(), isMethodFinal, m.isStatic(),
                    m.isSynthetic(), m.isAbstract(), m.isSynchronized(), m.isNative(),
                    m.isDefault(), false, "method", m.flatSignature(),
                    Converter.obtainMethod(m.overriddenMethod()),
                    Converter.obtainType(m.returnType()),
                    new ArrayList<ParameterInfo>(Arrays.asList(
                            Converter.convertParameters(m.parameters(), m))),
                    new ArrayList<ClassInfo>(Arrays.asList(
                            Converter.convertClasses(m.thrownExceptions()))),
                    Converter.convertSourcePosition(m.position()),
                    new ArrayList<AnnotationInstanceInfo>(Arrays.asList(
                            Converter.convertAnnotationInstances(m.annotations()))));
        result.setVarargs(m.isVarArgs());
        result.init(null);
        return result;
      } else {
        ConstructorDoc m = (ConstructorDoc) o;
        // Workaround for a JavaDoc behavior change introduced in OpenJDK 8 that breaks
        // links in documentation and the content of API files like current.txt.
        // http://b/18051133.
        String name = m.name();
        ClassDoc containingClass = m.containingClass();
        if (containingClass.containingClass() != null) {
          // This should detect the new behavior and be bypassed otherwise.
          if (!name.contains(".")) {
            // Constructors of inner classes do not contain the name of the enclosing class
            // with OpenJDK 8. This simulates the old behavior:
            name = containingClass.name();
          }
        }
        // End of workaround.
        MethodInfo result =
            new MethodInfo(m.getRawCommentText(), new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(m.typeParameters()))),
                name, m.signature(), Converter.obtainClass(m.containingClass()), Converter
                .obtainClass(m.containingClass()), m.isPublic(), m.isProtected(), m
                .isPackagePrivate(), m.isPrivate(), m.isFinal(), m.isStatic(), m.isSynthetic(),
                false, m.isSynchronized(), m.isNative(), false/*isDefault*/, false, "constructor", m.flatSignature(),
                null, null, new ArrayList<ParameterInfo>(Arrays.asList(Converter.convertParameters(m.parameters(), m))),
                new ArrayList<ClassInfo>(Arrays.asList(Converter.convertClasses(m.thrownExceptions()))), Converter.convertSourcePosition(m
                    .position()), new ArrayList<AnnotationInstanceInfo>(Arrays.asList(Converter.convertAnnotationInstances(m.annotations()))));
        result.setVarargs(m.isVarArgs());
        result.init(null);
        return result;
      }
    }
  };


  private static FieldInfo[] convertFields(FieldDoc[] fields) {
    if (fields == null) return null;
    ArrayList<FieldInfo> out = new ArrayList<FieldInfo>();
    int N = fields.length;
    for (int i = 0; i < N; i++) {
      FieldInfo f = Converter.obtainField(fields[i]);
      if (f.checkLevel()) {
        out.add(f);
      }
    }
    return out.toArray(new FieldInfo[out.size()]);
  }

  private static FieldInfo obtainField(FieldDoc o) {
    return (FieldInfo) mFields.obtain(o);
  }

  private static FieldInfo obtainField(ConstructorDoc o) {
    return (FieldInfo) mFields.obtain(o);
  }

  private static Cache mFields = new Cache() {
    @Override
    protected Object make(Object o) {
      FieldDoc f = (FieldDoc) o;
      return new FieldInfo(f.name(), Converter.obtainClass(f.containingClass()), Converter
          .obtainClass(f.containingClass()), f.isPublic(), f.isProtected(), f.isPackagePrivate(), f
          .isPrivate(), f.isFinal(), f.isStatic(), f.isTransient(), f.isVolatile(),
          f.isSynthetic(), Converter.obtainType(f.type()), f.getRawCommentText(),
          f.constantValue(), Converter.convertSourcePosition(f.position()),
          new ArrayList<AnnotationInstanceInfo>(Arrays.asList(Converter
              .convertAnnotationInstances(f.annotations()))));
    }
  };

  private static PackageInfo obtainPackage(PackageDoc o) {
    return (PackageInfo) mPackagees.obtain(o);
  }

  private static Cache mPackagees = new Cache() {
    @Override
    protected Object make(Object o) {
      PackageDoc p = (PackageDoc) o;
      return new PackageInfo(p, p.name(), Converter.convertSourcePosition(p.position()));
    }
  };

  private static TypeInfo obtainType(Type o) {
    return (TypeInfo) mTypes.obtain(o);
  }

  private static Cache mTypes = new Cache() {
    @Override
    protected Object make(Object o) {
      Type t = (Type) o;
      String simpleTypeName;
      if (t instanceof ClassDoc) {
        simpleTypeName = ((ClassDoc) t).name();
      } else {
        simpleTypeName = t.simpleTypeName();
      }
      TypeInfo ti =
          new TypeInfo(t.isPrimitive(), t.dimension(), simpleTypeName, t.qualifiedTypeName(),
              Converter.obtainClass(t.asClassDoc()));
      return ti;
    }

    @Override
    protected void made(Object o, Object r) {
      Type t = (Type) o;
      TypeInfo ti = (TypeInfo) r;
      if (t.asParameterizedType() != null) {
        ti.setTypeArguments(new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(t.asParameterizedType().typeArguments()))));
      } else if (t instanceof ClassDoc) {
        ti.setTypeArguments(new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(((ClassDoc) t).typeParameters()))));
      } else if (t.asTypeVariable() != null) {
        ti.setBounds(null, new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes((t.asTypeVariable().bounds())))));
        ti.setIsTypeVariable(true);
      } else if (t.asWildcardType() != null) {
        ti.setIsWildcard(true);
        ti.setBounds(new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(t.asWildcardType().superBounds()))),
                new ArrayList<TypeInfo>(Arrays.asList(Converter.convertTypes(t.asWildcardType().extendsBounds()))));
      }
    }

    @Override
    protected Object keyFor(Object o) {
      Type t = (Type) o;
      while (t.asAnnotatedType() != null) {
        t = t.asAnnotatedType().underlyingType();
      }
      String keyString = t.getClass().getName() + "/" + t.toString() + "/";
      if (t.asParameterizedType() != null) {
        keyString += t.asParameterizedType().toString() + "/";
        if (t.asParameterizedType().typeArguments() != null) {
          for (Type ty : t.asParameterizedType().typeArguments()) {
            keyString += ty.toString() + "/";
          }
        }
      } else {
        keyString += "NoParameterizedType//";
      }
      if (t.asTypeVariable() != null) {
        keyString += t.asTypeVariable().toString() + "/";
        if (t.asTypeVariable().bounds() != null) {
          for (Type ty : t.asTypeVariable().bounds()) {
            keyString += ty.toString() + "/";
          }
        }
      } else {
        keyString += "NoTypeVariable//";
      }
      if (t.asWildcardType() != null) {
        keyString += t.asWildcardType().toString() + "/";
        if (t.asWildcardType().superBounds() != null) {
          for (Type ty : t.asWildcardType().superBounds()) {
            keyString += ty.toString() + "/";
          }
        }
        if (t.asWildcardType().extendsBounds() != null) {
          for (Type ty : t.asWildcardType().extendsBounds()) {
            keyString += ty.toString() + "/";
          }
        }
      } else {
        keyString += "NoWildCardType//";
      }

      return keyString;
    }
  };

  public static TypeInfo obtainTypeFromString(String type) {
    return (TypeInfo) mTypesFromString.obtain(type);
  }

  private static final Cache mTypesFromString = new Cache() {
    @Override
    protected Object make(Object o) {
      String name = (String) o;
      return new TypeInfo(name);
    }

    @Override
    protected void made(Object o, Object r) {

    }

    @Override
    protected Object keyFor(Object o) {
      return o;
    }
  };

  private static MemberInfo obtainMember(MemberDoc o) {
    return (MemberInfo) mMembers.obtain(o);
  }

  private static Cache mMembers = new Cache() {
    @Override
    protected Object make(Object o) {
      if (o instanceof MethodDoc) {
        return Converter.obtainMethod((MethodDoc) o);
      } else if (o instanceof ConstructorDoc) {
        return Converter.obtainMethod((ConstructorDoc) o);
      } else if (o instanceof FieldDoc) {
        return Converter.obtainField((FieldDoc) o);
      } else {
        return null;
      }
    }
  };

  private static AnnotationInstanceInfo[] convertAnnotationInstances(AnnotationDesc[] orig) {
    int len = orig.length;
    AnnotationInstanceInfo[] out = new AnnotationInstanceInfo[len];
    for (int i = 0; i < len; i++) {
      out[i] = Converter.obtainAnnotationInstance(orig[i]);
    }
    return out;
  }


  private static AnnotationInstanceInfo obtainAnnotationInstance(AnnotationDesc o) {
    return (AnnotationInstanceInfo) mAnnotationInstances.obtain(o);
  }

  private static Cache mAnnotationInstances = new Cache() {
    @Override
    protected Object make(Object o) {
      AnnotationDesc a = (AnnotationDesc) o;
      ClassInfo annotationType = Converter.obtainClass(a.annotationType());
      AnnotationDesc.ElementValuePair[] ev = a.elementValues();
      AnnotationValueInfo[] elementValues = new AnnotationValueInfo[ev.length];
      for (int i = 0; i < ev.length; i++) {
        elementValues[i] =
            obtainAnnotationValue(ev[i].value(), Converter.obtainMethod(ev[i].element()));
      }
      return new AnnotationInstanceInfo(annotationType, elementValues);
    }
  };


  private abstract static class Cache {
    void put(Object key, Object value) {
      mCache.put(key, value);
    }

    Object obtain(Object o) {
      if (o == null) {
        return null;
      }
      Object k = keyFor(o);
      Object r = mCache.get(k);
      if (r == null) {
        r = make(o);
        mCache.put(k, r);
        made(o, r);
      }
      return r;
    }

    protected HashMap<Object, Object> mCache = new HashMap<Object, Object>();

    protected abstract Object make(Object o);

    protected void made(Object o, Object r) {}

    protected Object keyFor(Object o) {
      return o;
    }

    Collection<?> all() {
      return null;
    }
  }

  // annotation values
  private static HashMap<AnnotationValue, AnnotationValueInfo> mAnnotationValues =
      new HashMap<AnnotationValue, AnnotationValueInfo>();
  private static HashSet<AnnotationValue> mAnnotationValuesNeedingInit =
      new HashSet<AnnotationValue>();

  private static AnnotationValueInfo obtainAnnotationValue(AnnotationValue o, MethodInfo element) {
    if (o == null) {
      return null;
    }
    AnnotationValueInfo v = mAnnotationValues.get(o);
    if (v != null) return v;
    v = new AnnotationValueInfo(element);
    mAnnotationValues.put(o, v);
    if (mAnnotationValuesNeedingInit != null) {
      mAnnotationValuesNeedingInit.add(o);
    } else {
      initAnnotationValue(o, v);
    }
    return v;
  }

  private static void initAnnotationValue(AnnotationValue o, AnnotationValueInfo v) {
    Object orig = o.value();
    Object converted;
    if (orig instanceof Type) {
      // class literal
      converted = Converter.obtainType((Type) orig);
    } else if (orig instanceof FieldDoc) {
      // enum constant
      converted = Converter.obtainField((FieldDoc) orig);
    } else if (orig instanceof AnnotationDesc) {
      // annotation instance
      converted = Converter.obtainAnnotationInstance((AnnotationDesc) orig);
    } else if (orig instanceof AnnotationValue[]) {
      AnnotationValue[] old = (AnnotationValue[]) orig;
      ArrayList<AnnotationValueInfo> values = new ArrayList<AnnotationValueInfo>();
      for (int i = 0; i < old.length; i++) {
        values.add(Converter.obtainAnnotationValue(old[i], null));
      }
      converted = values;
    } else {
      converted = orig;
    }
    v.init(converted);
  }

  private static void finishAnnotationValueInit() {
    int depth = 0;
    while (mAnnotationValuesNeedingInit.size() > 0) {
      HashSet<AnnotationValue> set = mAnnotationValuesNeedingInit;
      mAnnotationValuesNeedingInit = new HashSet<AnnotationValue>();
      for (AnnotationValue o : set) {
        AnnotationValueInfo v = mAnnotationValues.get(o);
        initAnnotationValue(o, v);
      }
      depth++;
    }
    mAnnotationValuesNeedingInit = null;
  }
}
