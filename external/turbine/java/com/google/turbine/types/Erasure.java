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

package com.google.turbine.types;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;
import com.google.turbine.binder.bound.SourceTypeBoundClass;
import com.google.turbine.binder.sym.TyVarSymbol;
import com.google.turbine.type.Type;
import com.google.turbine.type.Type.TyVar;

/** Generic type erasure. */
public class Erasure {
  public static Type erase(Type ty, Function<TyVarSymbol, SourceTypeBoundClass.TyVarInfo> tenv) {
    switch (ty.tyKind()) {
      case PRIM_TY:
      case VOID_TY:
        return ty;
      case CLASS_TY:
        return eraseClassTy((Type.ClassTy) ty);
      case ARRAY_TY:
        return eraseArrayTy((Type.ArrayTy) ty, tenv);
      case TY_VAR:
        return eraseTyVar((TyVar) ty, tenv);
      default:
        throw new AssertionError(ty.tyKind());
    }
  }

  private static Type eraseTyVar(
      TyVar ty, Function<TyVarSymbol, SourceTypeBoundClass.TyVarInfo> tenv) {
    SourceTypeBoundClass.TyVarInfo info = tenv.apply(ty.sym());
    if (info.superClassBound() != null) {
      return erase(info.superClassBound(), tenv);
    }
    if (!info.interfaceBounds().isEmpty()) {
      return erase(info.interfaceBounds().get(0), tenv);
    }
    return Type.ClassTy.OBJECT;
  }

  private static Type.ArrayTy eraseArrayTy(
      Type.ArrayTy ty, Function<TyVarSymbol, SourceTypeBoundClass.TyVarInfo> tenv) {
    return new Type.ArrayTy(erase(ty.elementType(), tenv), ty.annos());
  }

  public static Type.ClassTy eraseClassTy(Type.ClassTy ty) {
    ImmutableList.Builder<Type.ClassTy.SimpleClassTy> classes = ImmutableList.builder();
    for (Type.ClassTy.SimpleClassTy c : ty.classes) {
      if (c.targs().isEmpty()) {
        classes.add(c);
      } else {
        classes.add(new Type.ClassTy.SimpleClassTy(c.sym(), ImmutableList.of(), c.annos()));
      }
    }
    return new Type.ClassTy(classes.build());
  }
}
