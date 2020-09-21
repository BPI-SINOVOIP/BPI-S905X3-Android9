/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.turbine.binder.bound;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;
import com.google.turbine.binder.lookup.CompoundScope;
import com.google.turbine.binder.lookup.MemberImportIndex;
import com.google.turbine.binder.sym.ClassSymbol;
import com.google.turbine.binder.sym.TyVarSymbol;
import com.google.turbine.diag.SourceFile;
import com.google.turbine.model.TurbineTyKind;
import com.google.turbine.type.AnnoInfo;
import com.google.turbine.type.Type;
import com.google.turbine.type.Type.ClassTy;
import javax.annotation.Nullable;

/** A HeaderBoundClass for classes compiled from source. */
public class SourceTypeBoundClass implements TypeBoundClass {

  private final TurbineTyKind kind;
  private final ClassSymbol owner;
  private final ImmutableMap<String, ClassSymbol> children;

  private final int access;
  private final ImmutableMap<String, TyVarSymbol> typeParameters;

  private final ImmutableMap<TyVarSymbol, TyVarInfo> typeParameterTypes;
  private final Type.ClassTy superClassType;
  private final ImmutableList<Type.ClassTy> interfaceTypes;
  private final ImmutableList<MethodInfo> methods;
  private final ImmutableList<FieldInfo> fields;
  private final CompoundScope enclosingScope;
  private final CompoundScope scope;
  private final MemberImportIndex memberImports;
  private final AnnotationMetadata annotationMetadata;
  private final ImmutableList<AnnoInfo> annotations;
  private final SourceFile source;

  public SourceTypeBoundClass(
      ImmutableList<ClassTy> interfaceTypes,
      ClassTy superClassType,
      ImmutableMap<TyVarSymbol, TyVarInfo> typeParameterTypes,
      int access,
      ImmutableList<MethodInfo> methods,
      ImmutableList<FieldInfo> fields,
      ClassSymbol owner,
      TurbineTyKind kind,
      ImmutableMap<String, ClassSymbol> children,
      ImmutableMap<String, TyVarSymbol> typeParameters,
      CompoundScope enclosingScope,
      CompoundScope scope,
      MemberImportIndex memberImports,
      AnnotationMetadata annotationMetadata,
      ImmutableList<AnnoInfo> annotations,
      SourceFile source) {
    this.interfaceTypes = interfaceTypes;
    this.superClassType = superClassType;
    this.typeParameterTypes = typeParameterTypes;
    this.access = access;
    this.methods = methods;
    this.fields = fields;
    this.owner = owner;
    this.kind = kind;
    this.children = children;
    this.typeParameters = typeParameters;
    this.enclosingScope = enclosingScope;
    this.scope = scope;
    this.memberImports = memberImports;
    this.annotationMetadata = annotationMetadata;
    this.annotations = annotations;
    this.source = source;
  }

  @Override
  public ClassSymbol superclass() {
    return superClassType() != null ? superClassType().sym() : null;
  }

  @Override
  public ImmutableList<ClassSymbol> interfaces() {
    return ImmutableList.copyOf(
        Iterables.transform(
            interfaceTypes,
            new Function<ClassTy, ClassSymbol>() {
              @Override
              public ClassSymbol apply(ClassTy classTy) {
                return classTy.sym();
              }
            }));
  }

  @Override
  public int access() {
    return access;
  }

  @Override
  public TurbineTyKind kind() {
    return kind;
  }

  @Nullable
  @Override
  public ClassSymbol owner() {
    return owner;
  }

  @Override
  public ImmutableMap<String, ClassSymbol> children() {
    return children;
  }

  @Override
  public ImmutableMap<String, TyVarSymbol> typeParameters() {
    return typeParameters;
  }

  /** Implemented interface types. */
  public ImmutableList<Type.ClassTy> interfaceTypes() {
    return interfaceTypes;
  }

  /** The super-class type. */
  @Override
  public Type.ClassTy superClassType() {
    return superClassType;
  }

  /** Declared methods. */
  @Override
  public ImmutableList<MethodInfo> methods() {
    return methods;
  }

  @Override
  public AnnotationMetadata annotationMetadata() {
    return annotationMetadata;
  }

  /** Declared fields. */
  @Override
  public ImmutableList<FieldInfo> fields() {
    return fields;
  }

  /** Declared type parameters. */
  @Override
  public ImmutableMap<TyVarSymbol, TyVarInfo> typeParameterTypes() {
    return typeParameterTypes;
  }

  /** The scope of the enclosing declaration or compilation unit. */
  public CompoundScope enclosingScope() {
    return enclosingScope;
  }

  /** The scope of the current class, including its members. */
  public CompoundScope scope() {
    return scope;
  }

  /** The static member import index for the enclosing compilation unit. */
  public MemberImportIndex memberImports() {
    return memberImports;
  }

  /** Declaration annotations. */
  public ImmutableList<AnnoInfo> annotations() {
    return annotations;
  }

  /** The source file. */
  public SourceFile source() {
    return source;
  }
}
