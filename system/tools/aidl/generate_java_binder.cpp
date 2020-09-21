/*
 * Copyright (C) 2016, The Android Open Source Project
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

#include "generate_java.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <unordered_set>

#include <android-base/macros.h>
#include <android-base/stringprintf.h>

#include "options.h"
#include "type_java.h"

using std::string;

using android::base::StringPrintf;

namespace android {
namespace aidl {
namespace java {

// =================================================
class StubClass : public Class {
 public:
  StubClass(const Type* type, const InterfaceType* interfaceType,
            JavaTypeNamespace* types);
  virtual ~StubClass() = default;

  Variable* transact_code;
  Variable* transact_data;
  Variable* transact_reply;
  Variable* transact_flags;
  SwitchStatement* transact_switch;
  StatementBlock* transact_statements;

  // Where onTransact cases should be generated as separate methods.
  bool transact_outline;
  // Specific methods that should be outlined when transact_outline is true.
  std::unordered_set<const AidlMethod*> outline_methods;
  // Number of all methods.
  size_t all_method_count;

  // Finish generation. This will add a default case to the switch.
  void finish();

  Expression* get_transact_descriptor(const JavaTypeNamespace* types,
                                      const AidlMethod* method);

 private:
  void make_as_interface(const InterfaceType* interfaceType,
                         JavaTypeNamespace* types);

  Variable* transact_descriptor;

  DISALLOW_COPY_AND_ASSIGN(StubClass);
};

StubClass::StubClass(const Type* type, const InterfaceType* interfaceType,
                     JavaTypeNamespace* types)
    : Class() {
  transact_descriptor = nullptr;
  transact_outline = false;
  all_method_count = 0;  // Will be set when outlining may be enabled.

  this->comment = "/** Local-side IPC implementation stub class. */";
  this->modifiers = PUBLIC | ABSTRACT | STATIC;
  this->what = Class::CLASS;
  this->type = type;
  this->extends = types->BinderNativeType();
  this->interfaces.push_back(interfaceType);

  // descriptor
  Field* descriptor =
      new Field(STATIC | FINAL | PRIVATE,
                new Variable(types->StringType(), "DESCRIPTOR"));
  descriptor->value = "\"" + interfaceType->JavaType() + "\"";
  this->elements.push_back(descriptor);

  // ctor
  Method* ctor = new Method;
  ctor->modifiers = PUBLIC;
  ctor->comment =
      "/** Construct the stub at attach it to the "
      "interface. */";
  ctor->name = "Stub";
  ctor->statements = new StatementBlock;
  MethodCall* attach =
      new MethodCall(THIS_VALUE, "attachInterface", 2, THIS_VALUE,
                     new LiteralExpression("DESCRIPTOR"));
  ctor->statements->Add(attach);
  this->elements.push_back(ctor);

  // asInterface
  make_as_interface(interfaceType, types);

  // asBinder
  Method* asBinder = new Method;
  asBinder->modifiers = PUBLIC | OVERRIDE;
  asBinder->returnType = types->IBinderType();
  asBinder->name = "asBinder";
  asBinder->statements = new StatementBlock;
  asBinder->statements->Add(new ReturnStatement(THIS_VALUE));
  this->elements.push_back(asBinder);

  // onTransact
  this->transact_code = new Variable(types->IntType(), "code");
  this->transact_data = new Variable(types->ParcelType(), "data");
  this->transact_reply = new Variable(types->ParcelType(), "reply");
  this->transact_flags = new Variable(types->IntType(), "flags");
  Method* onTransact = new Method;
  onTransact->modifiers = PUBLIC | OVERRIDE;
  onTransact->returnType = types->BoolType();
  onTransact->name = "onTransact";
  onTransact->parameters.push_back(this->transact_code);
  onTransact->parameters.push_back(this->transact_data);
  onTransact->parameters.push_back(this->transact_reply);
  onTransact->parameters.push_back(this->transact_flags);
  onTransact->statements = new StatementBlock;
  transact_statements = onTransact->statements;
  onTransact->exceptions.push_back(types->RemoteExceptionType());
  this->elements.push_back(onTransact);
  this->transact_switch = new SwitchStatement(this->transact_code);
}

void StubClass::finish() {
  Case* default_case = new Case;

  MethodCall* superCall = new MethodCall(
        SUPER_VALUE, "onTransact", 4, this->transact_code, this->transact_data,
        this->transact_reply, this->transact_flags);
  default_case->statements->Add(new ReturnStatement(superCall));
  transact_switch->cases.push_back(default_case);

  transact_statements->Add(this->transact_switch);
}

// The the expression for the interface's descriptor to be used when
// generating code for the given method. Null is acceptable for method
// and stands for synthetic cases.
Expression* StubClass::get_transact_descriptor(const JavaTypeNamespace* types,
                                               const AidlMethod* method) {
  if (transact_outline) {
    if (method != nullptr) {
      // When outlining, each outlined method needs its own literal.
      if (outline_methods.count(method) != 0) {
        return new LiteralExpression("DESCRIPTOR");
      }
    } else {
      // Synthetic case. A small number is assumed. Use its own descriptor
      // if there are only synthetic cases.
      if (outline_methods.size() == all_method_count) {
        return new LiteralExpression("DESCRIPTOR");
      }
    }
  }

  // When not outlining, store the descriptor literal into a local variable, in
  // an effort to save const-string instructions in each switch case.
  if (transact_descriptor == nullptr) {
    transact_descriptor = new Variable(types->StringType(), "descriptor");
    transact_statements->Add(
        new VariableDeclaration(transact_descriptor,
                                new LiteralExpression("DESCRIPTOR")));
  }
  return transact_descriptor;
}

void StubClass::make_as_interface(const InterfaceType* interfaceType,
                                  JavaTypeNamespace* types) {
  Variable* obj = new Variable(types->IBinderType(), "obj");

  Method* m = new Method;
  m->comment = "/**\n * Cast an IBinder object into an ";
  m->comment += interfaceType->JavaType();
  m->comment += " interface,\n";
  m->comment += " * generating a proxy if needed.\n */";
  m->modifiers = PUBLIC | STATIC;
  m->returnType = interfaceType;
  m->name = "asInterface";
  m->parameters.push_back(obj);
  m->statements = new StatementBlock;

  IfStatement* ifstatement = new IfStatement();
  ifstatement->expression = new Comparison(obj, "==", NULL_VALUE);
  ifstatement->statements = new StatementBlock;
  ifstatement->statements->Add(new ReturnStatement(NULL_VALUE));
  m->statements->Add(ifstatement);

  // IInterface iin = obj.queryLocalInterface(DESCRIPTOR)
  MethodCall* queryLocalInterface = new MethodCall(obj, "queryLocalInterface");
  queryLocalInterface->arguments.push_back(new LiteralExpression("DESCRIPTOR"));
  IInterfaceType* iinType = new IInterfaceType(types);
  Variable* iin = new Variable(iinType, "iin");
  VariableDeclaration* iinVd =
      new VariableDeclaration(iin, queryLocalInterface, NULL);
  m->statements->Add(iinVd);

  // Ensure the instance type of the local object is as expected.
  // One scenario where this is needed is if another package (with a
  // different class loader) runs in the same process as the service.

  // if (iin != null && iin instanceof <interfaceType>) return (<interfaceType>)
  // iin;
  Comparison* iinNotNull = new Comparison(iin, "!=", NULL_VALUE);
  Comparison* instOfCheck =
      new Comparison(iin, " instanceof ",
                     new LiteralExpression(interfaceType->JavaType()));
  IfStatement* instOfStatement = new IfStatement();
  instOfStatement->expression = new Comparison(iinNotNull, "&&", instOfCheck);
  instOfStatement->statements = new StatementBlock;
  instOfStatement->statements->Add(
      new ReturnStatement(new Cast(interfaceType, iin)));
  m->statements->Add(instOfStatement);

  NewExpression* ne = new NewExpression(interfaceType->GetProxy());
  ne->arguments.push_back(obj);
  m->statements->Add(new ReturnStatement(ne));

  this->elements.push_back(m);
}

// =================================================
class ProxyClass : public Class {
 public:
  ProxyClass(const JavaTypeNamespace* types, const Type* type,
             const InterfaceType* interfaceType);
  virtual ~ProxyClass();

  Variable* mRemote;
  bool mOneWay;
};

ProxyClass::ProxyClass(const JavaTypeNamespace* types, const Type* type,
                       const InterfaceType* interfaceType)
    : Class() {
  this->modifiers = PRIVATE | STATIC;
  this->what = Class::CLASS;
  this->type = type;
  this->interfaces.push_back(interfaceType);

  mOneWay = interfaceType->OneWay();

  // IBinder mRemote
  mRemote = new Variable(types->IBinderType(), "mRemote");
  this->elements.push_back(new Field(PRIVATE, mRemote));

  // Proxy()
  Variable* remote = new Variable(types->IBinderType(), "remote");
  Method* ctor = new Method;
  ctor->name = "Proxy";
  ctor->statements = new StatementBlock;
  ctor->parameters.push_back(remote);
  ctor->statements->Add(new Assignment(mRemote, remote));
  this->elements.push_back(ctor);

  // IBinder asBinder()
  Method* asBinder = new Method;
  asBinder->modifiers = PUBLIC | OVERRIDE;
  asBinder->returnType = types->IBinderType();
  asBinder->name = "asBinder";
  asBinder->statements = new StatementBlock;
  asBinder->statements->Add(new ReturnStatement(mRemote));
  this->elements.push_back(asBinder);
}

ProxyClass::~ProxyClass() {}

// =================================================
static void generate_new_array(const Type* t, StatementBlock* addTo,
                               Variable* v, Variable* parcel,
                               JavaTypeNamespace* types) {
  Variable* len = new Variable(types->IntType(), v->name + "_length");
  addTo->Add(new VariableDeclaration(len, new MethodCall(parcel, "readInt")));
  IfStatement* lencheck = new IfStatement();
  lencheck->expression = new Comparison(len, "<", new LiteralExpression("0"));
  lencheck->statements->Add(new Assignment(v, NULL_VALUE));
  lencheck->elseif = new IfStatement();
  lencheck->elseif->statements->Add(
      new Assignment(v, new NewArrayExpression(t, len)));
  addTo->Add(lencheck);
}

static void generate_write_to_parcel(const Type* t, StatementBlock* addTo,
                                     Variable* v, Variable* parcel, int flags) {
  t->WriteToParcel(addTo, v, parcel, flags);
}

static void generate_create_from_parcel(const Type* t, StatementBlock* addTo,
                                        Variable* v, Variable* parcel,
                                        Variable** cl) {
  t->CreateFromParcel(addTo, v, parcel, cl);
}

static void generate_int_constant(const AidlIntConstant& constant,
                                  Class* interface) {
  IntConstant* decl = new IntConstant(constant.GetName(), constant.GetValue());
  interface->elements.push_back(decl);
}

static void generate_string_constant(const AidlStringConstant& constant,
                                     Class* interface) {
  StringConstant* decl = new StringConstant(constant.GetName(),
                                            constant.GetValue());
  interface->elements.push_back(decl);
}

static std::unique_ptr<Method> generate_interface_method(
    const AidlMethod& method, JavaTypeNamespace* types) {
  std::unique_ptr<Method> decl(new Method);
  decl->comment = method.GetComments();
  decl->modifiers = PUBLIC;
  decl->returnType = method.GetType().GetLanguageType<Type>();
  decl->returnTypeDimension = method.GetType().IsArray() ? 1 : 0;
  decl->name = method.GetName();

  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    decl->parameters.push_back(
        new Variable(arg->GetType().GetLanguageType<Type>(), arg->GetName(),
                     arg->GetType().IsArray() ? 1 : 0));
  }

  decl->exceptions.push_back(types->RemoteExceptionType());

  return decl;
}

static void generate_stub_code(const AidlInterface& iface,
                               const AidlMethod& method,
                               const std::string& transactCodeName,
                               bool oneway,
                               Variable* transact_data,
                               Variable* transact_reply,
                               JavaTypeNamespace* types,
                               StatementBlock* statements,
                               StubClass* stubClass) {
  TryStatement* tryStatement = nullptr;
  FinallyStatement* finallyStatement = nullptr;
  MethodCall* realCall = new MethodCall(THIS_VALUE, method.GetName());

  // interface token validation is the very first thing we do
  statements->Add(new MethodCall(transact_data,
                                 "enforceInterface", 1,
                                 stubClass->get_transact_descriptor(types,
                                                                    &method)));

  // args
  VariableFactory stubArgs("_arg");
  {
    Variable* cl = NULL;
    for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
      const Type* t = arg->GetType().GetLanguageType<Type>();
      Variable* v = stubArgs.Get(t);
      v->dimension = arg->GetType().IsArray() ? 1 : 0;

      statements->Add(new VariableDeclaration(v));

      if (arg->GetDirection() & AidlArgument::IN_DIR) {
        generate_create_from_parcel(t,
                                    statements,
                                    v,
                                    transact_data,
                                    &cl);
      } else {
        if (!arg->GetType().IsArray()) {
          statements->Add(new Assignment(v, new NewExpression(v->type)));
        } else {
          generate_new_array(v->type,
                             statements,
                             v,
                             transact_data,
                             types);
        }
      }

      realCall->arguments.push_back(v);
    }
  }

  if (iface.ShouldGenerateTraces()) {
    // try and finally, but only when generating trace code
    tryStatement = new TryStatement();
    finallyStatement = new FinallyStatement();

    tryStatement->statements->Add(new MethodCall(
        new LiteralExpression("android.os.Trace"), "traceBegin", 2,
        new LiteralExpression("android.os.Trace.TRACE_TAG_AIDL"),
        new StringLiteralExpression(iface.GetName() + "::"
            + method.GetName() + "::server")));

    finallyStatement->statements->Add(new MethodCall(
        new LiteralExpression("android.os.Trace"), "traceEnd", 1,
        new LiteralExpression("android.os.Trace.TRACE_TAG_AIDL")));
  }

  // the real call
  if (method.GetType().GetName() == "void") {
    if (iface.ShouldGenerateTraces()) {
      statements->Add(tryStatement);
      tryStatement->statements->Add(realCall);
      statements->Add(finallyStatement);
    } else {
      statements->Add(realCall);
    }

    if (!oneway) {
      // report that there were no exceptions
      MethodCall* ex =
          new MethodCall(transact_reply, "writeNoException", 0);
      statements->Add(ex);
    }
  } else {
    Variable* _result =
        new Variable(method.GetType().GetLanguageType<Type>(),
                     "_result",
                     method.GetType().IsArray() ? 1 : 0);
    if (iface.ShouldGenerateTraces()) {
      statements->Add(new VariableDeclaration(_result));
      statements->Add(tryStatement);
      tryStatement->statements->Add(new Assignment(_result, realCall));
      statements->Add(finallyStatement);
    } else {
      statements->Add(new VariableDeclaration(_result, realCall));
    }

    if (!oneway) {
      // report that there were no exceptions
      MethodCall* ex =
          new MethodCall(transact_reply, "writeNoException", 0);
      statements->Add(ex);
    }

    // marshall the return value
    generate_write_to_parcel(method.GetType().GetLanguageType<Type>(),
                             statements,
                             _result,
                             transact_reply,
                             Type::PARCELABLE_WRITE_RETURN_VALUE);
  }

  // out parameters
  int i = 0;
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    const Type* t = arg->GetType().GetLanguageType<Type>();
    Variable* v = stubArgs.Get(i++);

    if (arg->GetDirection() & AidlArgument::OUT_DIR) {
      generate_write_to_parcel(t,
                               statements,
                               v,
                               transact_reply,
                               Type::PARCELABLE_WRITE_RETURN_VALUE);
    }
  }

  // return true
  statements->Add(new ReturnStatement(TRUE_VALUE));
}


static void generate_stub_case(const AidlInterface& iface,
                               const AidlMethod& method,
                               const std::string& transactCodeName,
                               bool oneway,
                               StubClass* stubClass,
                               JavaTypeNamespace* types) {
  Case* c = new Case(transactCodeName);

  generate_stub_code(iface,
                     method,
                     transactCodeName,
                     oneway,
                     stubClass->transact_data,
                     stubClass->transact_reply,
                     types,
                     c->statements,
                     stubClass);

  stubClass->transact_switch->cases.push_back(c);
}

static void generate_stub_case_outline(const AidlInterface& iface,
                                       const AidlMethod& method,
                                       const std::string& transactCodeName,
                                       bool oneway,
                                       StubClass* stubClass,
                                       JavaTypeNamespace* types) {
  std::string outline_name = "onTransact$" + method.GetName() + "$";
  // Generate an "outlined" method with the actual code.
  {
    Variable* transact_data = new Variable(types->ParcelType(), "data");
    Variable* transact_reply = new Variable(types->ParcelType(), "reply");
    Method* onTransact_case = new Method;
    onTransact_case->modifiers = PRIVATE;
    onTransact_case->returnType = types->BoolType();
    onTransact_case->name = outline_name;
    onTransact_case->parameters.push_back(transact_data);
    onTransact_case->parameters.push_back(transact_reply);
    onTransact_case->statements = new StatementBlock;
    onTransact_case->exceptions.push_back(types->RemoteExceptionType());
    stubClass->elements.push_back(onTransact_case);

    generate_stub_code(iface,
                       method,
                       transactCodeName,
                       oneway,
                       transact_data,
                       transact_reply,
                       types,
                       onTransact_case->statements,
                       stubClass);
  }

  // Generate the case dispatch.
  {
    Case* c = new Case(transactCodeName);

    MethodCall* helper_call = new MethodCall(THIS_VALUE,
                                             outline_name,
                                             2,
                                             stubClass->transact_data,
                                             stubClass->transact_reply);
    c->statements->Add(new ReturnStatement(helper_call));

    stubClass->transact_switch->cases.push_back(c);
  }
}

static std::unique_ptr<Method> generate_proxy_method(
    const AidlInterface& iface,
    const AidlMethod& method,
    const std::string& transactCodeName,
    bool oneway,
    ProxyClass* proxyClass,
    JavaTypeNamespace* types) {
  std::unique_ptr<Method> proxy(new Method);
  proxy->comment = method.GetComments();
  proxy->modifiers = PUBLIC | OVERRIDE;
  proxy->returnType = method.GetType().GetLanguageType<Type>();
  proxy->returnTypeDimension = method.GetType().IsArray() ? 1 : 0;
  proxy->name = method.GetName();
  proxy->statements = new StatementBlock;
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    proxy->parameters.push_back(
        new Variable(arg->GetType().GetLanguageType<Type>(), arg->GetName(),
                     arg->GetType().IsArray() ? 1 : 0));
  }
  proxy->exceptions.push_back(types->RemoteExceptionType());

  // the parcels
  Variable* _data = new Variable(types->ParcelType(), "_data");
  proxy->statements->Add(new VariableDeclaration(
      _data, new MethodCall(types->ParcelType(), "obtain")));
  Variable* _reply = NULL;
  if (!oneway) {
    _reply = new Variable(types->ParcelType(), "_reply");
    proxy->statements->Add(new VariableDeclaration(
        _reply, new MethodCall(types->ParcelType(), "obtain")));
  }

  // the return value
  Variable* _result = NULL;
  if (method.GetType().GetName() != "void") {
    _result = new Variable(proxy->returnType, "_result",
                           method.GetType().IsArray() ? 1 : 0);
    proxy->statements->Add(new VariableDeclaration(_result));
  }

  // try and finally
  TryStatement* tryStatement = new TryStatement();
  proxy->statements->Add(tryStatement);
  FinallyStatement* finallyStatement = new FinallyStatement();
  proxy->statements->Add(finallyStatement);

  if (iface.ShouldGenerateTraces()) {
    tryStatement->statements->Add(new MethodCall(
          new LiteralExpression("android.os.Trace"), "traceBegin", 2,
          new LiteralExpression("android.os.Trace.TRACE_TAG_AIDL"),
          new StringLiteralExpression(iface.GetName() + "::" +
                                      method.GetName() + "::client")));
  }

  // the interface identifier token: the DESCRIPTOR constant, marshalled as a
  // string
  tryStatement->statements->Add(new MethodCall(
      _data, "writeInterfaceToken", 1, new LiteralExpression("DESCRIPTOR")));

  // the parameters
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    const Type* t = arg->GetType().GetLanguageType<Type>();
    Variable* v =
        new Variable(t, arg->GetName(), arg->GetType().IsArray() ? 1 : 0);
    AidlArgument::Direction dir = arg->GetDirection();
    if (dir == AidlArgument::OUT_DIR && arg->GetType().IsArray()) {
      IfStatement* checklen = new IfStatement();
      checklen->expression = new Comparison(v, "==", NULL_VALUE);
      checklen->statements->Add(
          new MethodCall(_data, "writeInt", 1, new LiteralExpression("-1")));
      checklen->elseif = new IfStatement();
      checklen->elseif->statements->Add(
          new MethodCall(_data, "writeInt", 1, new FieldVariable(v, "length")));
      tryStatement->statements->Add(checklen);
    } else if (dir & AidlArgument::IN_DIR) {
      generate_write_to_parcel(t, tryStatement->statements, v, _data, 0);
    } else {
      delete v;
    }
  }

  // the transact call
  MethodCall* call = new MethodCall(
      proxyClass->mRemote, "transact", 4,
      new LiteralExpression("Stub." + transactCodeName), _data,
      _reply ? _reply : NULL_VALUE,
          new LiteralExpression(oneway ? "android.os.IBinder.FLAG_ONEWAY" : "0"));
  tryStatement->statements->Add(call);

  // throw back exceptions.
  if (_reply) {
    MethodCall* ex = new MethodCall(_reply, "readException", 0);
    tryStatement->statements->Add(ex);
  }

  // returning and cleanup
  if (_reply != NULL) {
    Variable* cl = nullptr;
    if (_result != NULL) {
      generate_create_from_parcel(proxy->returnType, tryStatement->statements,
                                  _result, _reply, &cl);
    }

    // the out/inout parameters
    for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
      const Type* t = arg->GetType().GetLanguageType<Type>();
      if (arg->GetDirection() & AidlArgument::OUT_DIR) {
        Variable* v =
            new Variable(t, arg->GetName(), arg->GetType().IsArray() ? 1 : 0);
        t->ReadFromParcel(tryStatement->statements, v, _reply, &cl);
      }
    }

    finallyStatement->statements->Add(new MethodCall(_reply, "recycle"));
  }
  finallyStatement->statements->Add(new MethodCall(_data, "recycle"));

  if (iface.ShouldGenerateTraces()) {
    finallyStatement->statements->Add(new MethodCall(
        new LiteralExpression("android.os.Trace"), "traceEnd", 1,
        new LiteralExpression("android.os.Trace.TRACE_TAG_AIDL")));
  }

  if (_result != NULL) {
    proxy->statements->Add(new ReturnStatement(_result));
  }

  return proxy;
}

static void generate_methods(const AidlInterface& iface,
                             const AidlMethod& method,
                             Class* interface,
                             StubClass* stubClass,
                             ProxyClass* proxyClass,
                             int index,
                             JavaTypeNamespace* types) {
  const bool oneway = proxyClass->mOneWay || method.IsOneway();

  // == the TRANSACT_ constant =============================================
  string transactCodeName = "TRANSACTION_";
  transactCodeName += method.GetName();

  Field* transactCode = new Field(
      STATIC | FINAL, new Variable(types->IntType(), transactCodeName));
  transactCode->value =
      StringPrintf("(android.os.IBinder.FIRST_CALL_TRANSACTION + %d)", index);
  stubClass->elements.push_back(transactCode);

  // == the declaration in the interface ===================================
  Method* decl = generate_interface_method(method, types).release();
  interface->elements.push_back(decl);

  // == the stub method ====================================================
  bool outline_stub = stubClass->transact_outline &&
      stubClass->outline_methods.count(&method) != 0;
  if (outline_stub) {
    generate_stub_case_outline(iface,
                               method,
                               transactCodeName,
                               oneway,
                               stubClass,
                               types);
  } else {
    generate_stub_case(iface, method, transactCodeName, oneway, stubClass, types);
  }

  // == the proxy method ===================================================
  Method* proxy = generate_proxy_method(iface,
                                        method,
                                        transactCodeName,
                                        oneway,
                                        proxyClass,
                                        types).release();
  proxyClass->elements.push_back(proxy);
}

static void generate_interface_descriptors(StubClass* stub, ProxyClass* proxy,
                                           const JavaTypeNamespace* types) {
  // the interface descriptor transaction handler
  Case* c = new Case("INTERFACE_TRANSACTION");
  c->statements->Add(new MethodCall(stub->transact_reply, "writeString", 1,
                                    stub->get_transact_descriptor(types,
                                                                  nullptr)));
  c->statements->Add(new ReturnStatement(TRUE_VALUE));
  stub->transact_switch->cases.push_back(c);

  // and the proxy-side method returning the descriptor directly
  Method* getDesc = new Method;
  getDesc->modifiers = PUBLIC;
  getDesc->returnType = types->StringType();
  getDesc->returnTypeDimension = 0;
  getDesc->name = "getInterfaceDescriptor";
  getDesc->statements = new StatementBlock;
  getDesc->statements->Add(
      new ReturnStatement(new LiteralExpression("DESCRIPTOR")));
  proxy->elements.push_back(getDesc);
}

// Check whether (some) methods in this interface should be "outlined," that
// is, have specific onTransact methods for certain cases. Set up StubClass
// metadata accordingly.
//
// Outlining will be enabled if the interface has more than outline_threshold
// methods. In that case, the methods are sorted by number of arguments
// (so that more "complex" methods come later), and the first non_outline_count
// number of methods not outlined (are kept in the onTransact() method).
//
// Requirements: non_outline_count <= outline_threshold.
static void compute_outline_methods(const AidlInterface* iface,
                                    StubClass* stub,
                                    size_t outline_threshold,
                                    size_t non_outline_count) {
  CHECK_LE(non_outline_count, outline_threshold);
  // We'll outline (create sub methods) if there are more than min_methods
  // cases.
  stub->transact_outline = iface->GetMethods().size() > outline_threshold;
  if (stub->transact_outline) {
    stub->all_method_count = iface->GetMethods().size();
    std::vector<const AidlMethod*> methods;
    methods.reserve(iface->GetMethods().size());
    for (const std::unique_ptr<AidlMethod>& ptr : iface->GetMethods()) {
      methods.push_back(ptr.get());
    }

    std::stable_sort(
        methods.begin(),
        methods.end(),
        [](const AidlMethod* m1, const AidlMethod* m2) {
          return m1->GetArguments().size() < m2->GetArguments().size();
        });

    stub->outline_methods.insert(methods.begin() + non_outline_count,
                                 methods.end());
  }
}

Class* generate_binder_interface_class(const AidlInterface* iface,
                                       JavaTypeNamespace* types,
                                       const JavaOptions& options) {
  const InterfaceType* interfaceType = iface->GetLanguageType<InterfaceType>();

  // the interface class
  Class* interface = new Class;
  interface->comment = iface->GetComments();
  interface->modifiers = PUBLIC;
  interface->what = Class::INTERFACE;
  interface->type = interfaceType;
  interface->interfaces.push_back(types->IInterfaceType());

  // the stub inner class
  StubClass* stub =
      new StubClass(interfaceType->GetStub(), interfaceType, types);
  interface->elements.push_back(stub);

  compute_outline_methods(iface,
                          stub,
                          options.onTransact_outline_threshold_,
                          options.onTransact_non_outline_count_);

  // the proxy inner class
  ProxyClass* proxy =
      new ProxyClass(types, interfaceType->GetProxy(), interfaceType);
  stub->elements.push_back(proxy);

  // stub and proxy support for getInterfaceDescriptor()
  generate_interface_descriptors(stub, proxy, types);

  // all the declared constants of the interface
  for (const auto& item : iface->GetIntConstants()) {
    generate_int_constant(*item, interface);
  }
  for (const auto& item : iface->GetStringConstants()) {
    generate_string_constant(*item, interface);
  }

  // all the declared methods of the interface

  for (const auto& item : iface->GetMethods()) {
    generate_methods(*iface,
                     *item,
                     interface,
                     stub,
                     proxy,
                     item->GetId(),
                     types);
  }
  stub->finish();

  return interface;
}

}  // namespace java
}  // namespace android
}  // namespace aidl
