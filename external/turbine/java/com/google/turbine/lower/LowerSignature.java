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

package com.google.turbine.lower;

import com.google.common.collect.ImmutableList;
import com.google.turbine.binder.bound.SourceTypeBoundClass;
import com.google.turbine.binder.bound.TypeBoundClass;
import com.google.turbine.binder.env.Env;
import com.google.turbine.binder.sym.ClassSymbol;
import com.google.turbine.binder.sym.TyVarSymbol;
import com.google.turbine.bytecode.sig.Sig;
import com.google.turbine.bytecode.sig.Sig.ClassSig;
import com.google.turbine.bytecode.sig.Sig.ClassTySig;
import com.google.turbine.bytecode.sig.Sig.LowerBoundTySig;
import com.google.turbine.bytecode.sig.Sig.MethodSig;
import com.google.turbine.bytecode.sig.Sig.SimpleClassTySig;
import com.google.turbine.bytecode.sig.Sig.TySig;
import com.google.turbine.bytecode.sig.Sig.UpperBoundTySig;
import com.google.turbine.bytecode.sig.SigWriter;
import com.google.turbine.model.TurbineFlag;
import com.google.turbine.type.Type;
import com.google.turbine.type.Type.ArrayTy;
import com.google.turbine.type.Type.ClassTy;
import com.google.turbine.type.Type.ClassTy.SimpleClassTy;
import com.google.turbine.type.Type.PrimTy;
import com.google.turbine.type.Type.TyVar;
import com.google.turbine.type.Type.WildTy;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

/** Translator from {@link Type}s to {@link Sig}natures. */
public class LowerSignature {

  final Set<ClassSymbol> classes = new LinkedHashSet<>();

  /** Translates types to signatures. */
  public Sig.TySig signature(Type ty) {
    switch (ty.tyKind()) {
      case CLASS_TY:
        return classTySig((Type.ClassTy) ty);
      case TY_VAR:
        return tyVarSig((TyVar) ty);
      case ARRAY_TY:
        return arrayTySig((ArrayTy) ty);
      case PRIM_TY:
        return refBaseTy((PrimTy) ty);
      case VOID_TY:
        return Sig.VOID;
      case WILD_TY:
        return wildTy((WildTy) ty);
      default:
        throw new AssertionError(ty.tyKind());
    }
  }

  private Sig.BaseTySig refBaseTy(PrimTy t) {
    return new Sig.BaseTySig(t.primkind());
  }

  private Sig.ArrayTySig arrayTySig(ArrayTy t) {
    return new Sig.ArrayTySig(signature(t.elementType()));
  }

  private Sig.TyVarSig tyVarSig(TyVar t) {
    return new Sig.TyVarSig(t.sym().name());
  }

  private ClassTySig classTySig(ClassTy t) {
    classes.add(t.sym());
    ImmutableList.Builder<SimpleClassTySig> classes = ImmutableList.builder();
    Iterator<SimpleClassTy> it = t.classes.iterator();
    SimpleClassTy curr = it.next();
    while (curr.targs().isEmpty() && it.hasNext()) {
      curr = it.next();
    }
    String pkg;
    String name;
    int idx = curr.sym().binaryName().lastIndexOf('/');
    if (idx == -1) {
      pkg = "";
      name = curr.sym().binaryName();
    } else {
      pkg = curr.sym().binaryName().substring(0, idx);
      name = curr.sym().binaryName().substring(idx + 1);
    }
    classes.add(new Sig.SimpleClassTySig(name, tyArgSigs(curr)));
    while (it.hasNext()) {
      SimpleClassTy outer = curr;
      curr = it.next();
      String shortname = curr.sym().binaryName().substring(outer.sym().binaryName().length() + 1);
      classes.add(new Sig.SimpleClassTySig(shortname, tyArgSigs(curr)));
    }
    return new ClassTySig(pkg, classes.build());
  }

  private ImmutableList<TySig> tyArgSigs(SimpleClassTy part) {
    ImmutableList.Builder<TySig> tyargs = ImmutableList.builder();
    for (Type targ : part.targs()) {
      tyargs.add(signature(targ));
    }
    return tyargs.build();
  }

  private TySig wildTy(WildTy ty) {
    switch (ty.boundKind()) {
      case NONE:
        return new Sig.WildTyArgSig();
      case UPPER:
        return new UpperBoundTySig(signature(((Type.WildUpperBoundedTy) ty).bound()));
      case LOWER:
        return new LowerBoundTySig(signature(((Type.WildLowerBoundedTy) ty).bound()));
      default:
        throw new AssertionError(ty.boundKind());
    }
  }

  /**
   * Produces a method signature attribute for a generic method, or {@code null} if the signature is
   * unnecessary.
   */
  public String methodSignature(
      Env<ClassSymbol, TypeBoundClass> env,
      SourceTypeBoundClass.MethodInfo method,
      ClassSymbol sym) {
    if (!needsMethodSig(sym, env, method)) {
      return null;
    }
    ImmutableList<Sig.TyParamSig> typarams = tyParamSig(method.tyParams());
    ImmutableList.Builder<Sig.TySig> fparams = ImmutableList.builder();
    for (SourceTypeBoundClass.ParamInfo t : method.parameters()) {
      if (t.synthetic()) {
        continue;
      }
      fparams.add(signature(t.type()));
    }
    Sig.TySig ret = signature(method.returnType());
    ImmutableList.Builder<Sig.TySig> excn = ImmutableList.builder();
    boolean needsExnSig = false;
    for (Type e : method.exceptions()) {
      if (needsSig(e)) {
        needsExnSig = true;
        break;
      }
    }
    if (needsExnSig) {
      for (Type e : method.exceptions()) {
        excn.add(signature(e));
      }
    }
    MethodSig sig = new MethodSig(typarams, fparams.build(), ret, excn.build());
    return SigWriter.method(sig);
  }

  private boolean needsMethodSig(
      ClassSymbol sym, Env<ClassSymbol, TypeBoundClass> env, SourceTypeBoundClass.MethodInfo m) {
    if ((env.get(sym).access() & TurbineFlag.ACC_ENUM) == TurbineFlag.ACC_ENUM
        && m.name().equals("<init>")) {
      // JDK-8024694: javac always expects signature attribute for enum constructors
      return true;
    }
    if ((m.access() & TurbineFlag.ACC_SYNTH_CTOR) == TurbineFlag.ACC_SYNTH_CTOR) {
      return false;
    }
    if (!m.tyParams().isEmpty()) {
      return true;
    }
    if (m.returnType() != null && needsSig(m.returnType())) {
      return true;
    }
    for (SourceTypeBoundClass.ParamInfo t : m.parameters()) {
      if (t.synthetic()) {
        continue;
      }
      if (needsSig(t.type())) {
        return true;
      }
    }
    for (Type t : m.exceptions()) {
      if (needsSig(t)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Produces a class signature attribute for a generic class, or {@code null} if the signature is
   * unnecessary.
   */
  public String classSignature(SourceTypeBoundClass info) {
    if (!classNeedsSig(info)) {
      return null;
    }
    ImmutableList<Sig.TyParamSig> typarams = tyParamSig(info.typeParameterTypes());

    ClassTySig xtnd = null;
    if (info.superClassType() != null) {
      xtnd = classTySig(info.superClassType());
    }
    ImmutableList.Builder<ClassTySig> impl = ImmutableList.builder();
    for (ClassTy i : info.interfaceTypes()) {
      impl.add(classTySig(i));
    }
    ClassSig sig = new ClassSig(typarams, xtnd, impl.build());
    return SigWriter.classSig(sig);
  }

  /**
   * A field signature, or {@code null} if the descriptor provides all necessary type information.
   */
  public String fieldSignature(Type type) {
    return needsSig(type) ? SigWriter.type(signature(type)) : null;
  }

  private boolean classNeedsSig(SourceTypeBoundClass ci) {
    if (!ci.typeParameters().isEmpty()) {
      return true;
    }
    if (ci.superClassType() != null && needsSig(ci.superClassType())) {
      return true;
    }
    for (ClassTy i : ci.interfaceTypes()) {
      if (needsSig(i)) {
        return true;
      }
    }
    return false;
  }

  private boolean needsSig(Type ty) {
    switch (ty.tyKind()) {
      case PRIM_TY:
      case VOID_TY:
        return false;
      case CLASS_TY:
        {
          for (SimpleClassTy s : ((ClassTy) ty).classes) {
            if (!s.targs().isEmpty()) {
              return true;
            }
          }
          return false;
        }
      case ARRAY_TY:
        return needsSig(((ArrayTy) ty).elementType());
      case TY_VAR:
        return true;
      default:
        throw new AssertionError(ty.tyKind());
    }
  }

  private ImmutableList<Sig.TyParamSig> tyParamSig(
      Map<TyVarSymbol, SourceTypeBoundClass.TyVarInfo> px) {
    ImmutableList.Builder<Sig.TyParamSig> result = ImmutableList.builder();
    for (Map.Entry<TyVarSymbol, SourceTypeBoundClass.TyVarInfo> entry : px.entrySet()) {
      result.add(tyParamSig(entry.getKey(), entry.getValue()));
    }
    return result.build();
  }

  private Sig.TyParamSig tyParamSig(TyVarSymbol sym, SourceTypeBoundClass.TyVarInfo info) {
    String identifier = sym.name();
    Sig.TySig cbound = null;
    if (info.superClassBound() != null) {
      cbound = signature(info.superClassBound());
    } else if (info.interfaceBounds().isEmpty()) {
      cbound =
          new ClassTySig(
              "java/lang", ImmutableList.of(new SimpleClassTySig("Object", ImmutableList.of())));
    }
    ImmutableList.Builder<Sig.TySig> ibounds = ImmutableList.builder();
    for (Type i : info.interfaceBounds()) {
      ibounds.add(signature(i));
    }
    return new Sig.TyParamSig(identifier, cbound, ibounds.build());
  }

  public String descriptor(ClassSymbol sym) {
    classes.add(sym);
    return sym.binaryName();
  }

  String objectType(ClassSymbol sym) {
    return "L" + descriptor(sym) + ";";
  }
}
