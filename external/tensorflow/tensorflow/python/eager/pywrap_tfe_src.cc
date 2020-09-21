/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <thread>

#include "tensorflow/python/eager/pywrap_tfe.h"

#include "tensorflow/c/c_api.h"
#include "tensorflow/c/c_api_internal.h"
#include "tensorflow/c/eager/c_api_internal.h"
#include "tensorflow/c/eager/tape.h"
#include "tensorflow/core/lib/gtl/cleanup.h"
#include "tensorflow/core/lib/gtl/compactptrset.h"
#include "tensorflow/core/lib/gtl/flatmap.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/mutex.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/python/eager/pywrap_tensor.h"

using tensorflow::string;
using tensorflow::strings::Printf;

namespace {

#define PARSE_VALUE(fn_name, type, check_fn, parse_fn)                       \
  bool fn_name(const string& key, PyObject* py_value, TF_Status* status,     \
               type* value) {                                                \
    if (check_fn(py_value)) {                                                \
      *value = static_cast<type>(parse_fn(py_value));                        \
      return true;                                                           \
    } else {                                                                 \
      TF_SetStatus(status, TF_INVALID_ARGUMENT,                              \
                   tensorflow::strings::StrCat(                              \
                       "Expecting " #type " value for attr ", key, ", got ", \
                       py_value->ob_type->tp_name)                           \
                       .c_str());                                            \
      return false;                                                          \
    }                                                                        \
  }

#if PY_MAJOR_VERSION >= 3
PARSE_VALUE(ParseIntValue, int, PyLong_Check, PyLong_AsLong)
PARSE_VALUE(ParseInt64Value, int64_t, PyLong_Check, PyLong_AsLong)
#else
PARSE_VALUE(ParseIntValue, int, PyInt_Check, PyInt_AsLong)
PARSE_VALUE(ParseInt64Value, int64_t, PyInt_Check, PyInt_AsLong)
PARSE_VALUE(ParseInt64LongValue, int64_t, PyLong_Check, PyLong_AsLong)
#endif
PARSE_VALUE(ParseFloatValue, float, PyFloat_Check, PyFloat_AsDouble)
#undef PARSE_VALUE

Py_ssize_t TensorShapeNumDims(PyObject* value) {
  const auto size = PySequence_Size(value);
  if (size == -1) {
    // TensorShape.__len__ raises an error in the scenario where the shape is an
    // unknown, which needs to be cleared.
    // TODO(nareshmodi): ensure that this is actually a TensorShape.
    PyErr_Clear();
  }
  return size;
}

bool ParseStringValue(const string& key, PyObject* py_value, TF_Status* status,
                      const char** value) {
  if (PyBytes_Check(py_value)) {
    *value = PyBytes_AsString(py_value);
    return true;
  }
#if PY_MAJOR_VERSION >= 3
  if (PyUnicode_Check(py_value)) {
    *value = PyUnicode_AsUTF8(py_value);
    return true;
  }
#endif
  TF_SetStatus(
      status, TF_INVALID_ARGUMENT,
      tensorflow::strings::StrCat("Expecting a string value for attr ", key,
                                  ", got ", py_value->ob_type->tp_name)
          .c_str());
  return false;
}

bool ParseBoolValue(const string& key, PyObject* py_value, TF_Status* status,
                    unsigned char* value) {
  *value = PyObject_IsTrue(py_value);
  return true;
}

bool IsInteger(PyObject* py_value) {
#if PY_MAJOR_VERSION >= 3
  return PyLong_Check(py_value);
#else
  return PyInt_Check(py_value);
#endif
}

// The passed in py_value is expected to be an object of the python type
// dtypes.DType or an int.
bool ParseTypeValue(const string& key, PyObject* py_value, TF_Status* status,
                    int* value) {
  if (IsInteger(py_value)) {
    return ParseIntValue(key, py_value, status, value);
  }

  PyObject* py_type_enum = PyObject_GetAttrString(py_value, "_type_enum");
  if (py_type_enum == nullptr) {
    return false;
  }

  if (!ParseIntValue(key, py_type_enum, status, value)) {
    Py_DECREF(py_type_enum);
    return false;
  }

  Py_DECREF(py_type_enum);
  return true;
}

bool SetOpAttrList(
    TFE_Op* op, const char* key, PyObject* py_list, TF_AttrType type,
    tensorflow::gtl::FlatMap<string, tensorflow::int64>* attr_list_sizes,
    TF_Status* status) {
  if (!PySequence_Check(py_list)) {
    TF_SetStatus(
        status, TF_INVALID_ARGUMENT,
        tensorflow::strings::StrCat("Expecting sequence value for attr ", key,
                                    ", got ", py_list->ob_type->tp_name)
            .c_str());
    return false;
  }
  const int num_values = PySequence_Size(py_list);
  if (attr_list_sizes != nullptr) (*attr_list_sizes)[key] = num_values;

#define PARSE_LIST(c_type, parse_fn)                                \
  std::unique_ptr<c_type[]> values(new c_type[num_values]);         \
  for (int i = 0; i < num_values; ++i) {                            \
    auto py_value = PySequence_ITEM(py_list, i);                    \
    if (!parse_fn(key, py_value, status, &values[i])) return false; \
  }

  if (type == TF_ATTR_STRING) {
    PARSE_LIST(const char*, ParseStringValue);
    TFE_OpSetAttrStringList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_INT) {
    PARSE_LIST(int64_t, ParseInt64Value);
    TFE_OpSetAttrIntList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_FLOAT) {
    PARSE_LIST(float, ParseFloatValue);
    TFE_OpSetAttrFloatList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_BOOL) {
    PARSE_LIST(unsigned char, ParseBoolValue);
    TFE_OpSetAttrBoolList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_TYPE) {
    PARSE_LIST(int, ParseTypeValue);
    TFE_OpSetAttrTypeList(op, key,
                          reinterpret_cast<const TF_DataType*>(values.get()),
                          num_values);
  } else if (type == TF_ATTR_SHAPE) {
    // Make one pass through the input counting the total number of
    // dims across all the input lists.
    int total_dims = 0;
    for (int i = 0; i < num_values; ++i) {
      auto py_value = PySequence_ITEM(py_list, i);
      if (py_value != Py_None) {
        if (!PySequence_Check(py_value)) {
          TF_SetStatus(
              status, TF_INVALID_ARGUMENT,
              tensorflow::strings::StrCat(
                  "Expecting None or sequence value for element", i,
                  " of attr ", key, ", got ", py_value->ob_type->tp_name)
                  .c_str());
          return false;
        }
        const auto size = TensorShapeNumDims(py_value);
        if (size >= 0) {
          total_dims += size;
        }
      }
    }
    // Allocate a buffer that can fit all of the dims together.
    std::unique_ptr<int64_t[]> buffer(new int64_t[total_dims]);
    // Copy the input dims into the buffer and set dims to point to
    // the start of each list's dims.
    std::unique_ptr<const int64_t* []> dims(new const int64_t*[num_values]);
    std::unique_ptr<int[]> num_dims(new int[num_values]);
    int64_t* offset = buffer.get();
    for (int i = 0; i < num_values; ++i) {
      auto py_value = PySequence_ITEM(py_list, i);
      if (py_value == Py_None) {
        dims[i] = nullptr;
        num_dims[i] = -1;
      } else {
        const auto size = TensorShapeNumDims(py_value);
        if (size == -1) {
          dims[i] = nullptr;
          num_dims[i] = -1;
          continue;
        }
        dims[i] = offset;
        num_dims[i] = size;
        for (int j = 0; j < size; ++j) {
          auto inner_py_value = PySequence_ITEM(py_value, j);
          if (inner_py_value == Py_None) {
            *offset = -1;
          } else if (!ParseInt64Value(key, inner_py_value, status, offset)) {
            return false;
          }
          ++offset;
        }
      }
    }
    TFE_OpSetAttrShapeList(op, key, dims.get(), num_dims.get(), num_values,
                           status);
    if (TF_GetCode(status) != TF_OK) return false;
  } else {
    TF_SetStatus(status, TF_UNIMPLEMENTED,
                 tensorflow::strings::StrCat("Attr ", key,
                                             " has unhandled list type ", type)
                     .c_str());
    return false;
  }
#undef PARSE_LIST
  return true;
}

// This is only declared here since GetFunc makes a recursive call to
// SetOpAttrScalarDefault.
void SetOpAttrScalarDefault(
    TFE_Context* ctx, TFE_Op* op, const tensorflow::AttrValue& default_value,
    const char* attr_name,
    tensorflow::gtl::FlatMap<string, tensorflow::int64>* attr_list_sizes,
    TF_Status* status);

TFE_Op* GetFunc(TFE_Context* ctx, const tensorflow::NameAttrList& func,
                TF_Status* status) {
  TFE_Op* func_op = TFE_NewOp(ctx, func.name().data(), status);
  for (const auto& attr : func.attr()) {
    if (TF_GetCode(status) != TF_OK) return nullptr;
    SetOpAttrScalarDefault(ctx, func_op, attr.second, attr.first.data(),
                           nullptr, status);
    if (TF_GetCode(status) != TF_OK) return nullptr;
  }
  return func_op;
}

void SetOpAttrListDefault(
    TFE_Context* ctx, TFE_Op* op, const tensorflow::OpDef::AttrDef& attr,
    const char* key, TF_AttrType type,
    tensorflow::gtl::FlatMap<string, tensorflow::int64>* attr_list_sizes,
    TF_Status* status) {
  if (type == TF_ATTR_STRING) {
    int num_values = attr.default_value().list().s_size();
    std::unique_ptr<const char* []> values(new const char*[num_values]);
    (*attr_list_sizes)[key] = num_values;
    for (int i = 0; i < num_values; i++) {
      values[i] = attr.default_value().list().s(i).data();
    }
    TFE_OpSetAttrStringList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_INT) {
    int num_values = attr.default_value().list().i_size();
    std::unique_ptr<int64_t[]> values(new int64_t[num_values]);
    (*attr_list_sizes)[key] = num_values;
    for (int i = 0; i < num_values; i++) {
      values[i] = attr.default_value().list().i(i);
    }
    TFE_OpSetAttrIntList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_FLOAT) {
    int num_values = attr.default_value().list().f_size();
    std::unique_ptr<float[]> values(new float[num_values]);
    (*attr_list_sizes)[key] = num_values;
    for (int i = 0; i < num_values; i++) {
      values[i] = attr.default_value().list().f(i);
    }
    TFE_OpSetAttrFloatList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_BOOL) {
    int num_values = attr.default_value().list().b_size();
    std::unique_ptr<unsigned char[]> values(new unsigned char[num_values]);
    (*attr_list_sizes)[key] = num_values;
    for (int i = 0; i < num_values; i++) {
      values[i] = attr.default_value().list().b(i);
    }
    TFE_OpSetAttrBoolList(op, key, values.get(), num_values);
  } else if (type == TF_ATTR_TYPE) {
    int num_values = attr.default_value().list().type_size();
    std::unique_ptr<int[]> values(new int[num_values]);
    (*attr_list_sizes)[key] = num_values;
    for (int i = 0; i < num_values; i++) {
      values[i] = attr.default_value().list().type(i);
    }
    TFE_OpSetAttrTypeList(op, key,
                          reinterpret_cast<const TF_DataType*>(values.get()),
                          attr.default_value().list().type_size());
  } else if (type == TF_ATTR_SHAPE) {
    int num_values = attr.default_value().list().shape_size();
    (*attr_list_sizes)[key] = num_values;
    int total_dims = 0;
    for (int i = 0; i < num_values; ++i) {
      if (!attr.default_value().list().shape(i).unknown_rank()) {
        total_dims += attr.default_value().list().shape(i).dim_size();
      }
    }
    // Allocate a buffer that can fit all of the dims together.
    std::unique_ptr<int64_t[]> buffer(new int64_t[total_dims]);
    // Copy the input dims into the buffer and set dims to point to
    // the start of each list's dims.
    std::unique_ptr<const int64_t* []> dims(new const int64_t*[num_values]);
    std::unique_ptr<int[]> num_dims(new int[num_values]);
    int64_t* offset = buffer.get();
    for (int i = 0; i < num_values; ++i) {
      const auto& shape = attr.default_value().list().shape(i);
      if (shape.unknown_rank()) {
        dims[i] = nullptr;
        num_dims[i] = -1;
      } else {
        for (int j = 0; j < shape.dim_size(); j++) {
          *offset = shape.dim(j).size();
          ++offset;
        }
      }
    }
    TFE_OpSetAttrShapeList(op, key, dims.get(), num_dims.get(), num_values,
                           status);
  } else if (type == TF_ATTR_FUNC) {
    int num_values = attr.default_value().list().func_size();
    (*attr_list_sizes)[key] = num_values;
    std::unique_ptr<const TFE_Op* []> funcs(new const TFE_Op*[num_values]);
    for (int i = 0; i < num_values; i++) {
      funcs[i] = GetFunc(ctx, attr.default_value().list().func(i), status);
    }
    TFE_OpSetAttrFunctionList(op, key, funcs.get(), num_values);
  } else {
    TF_SetStatus(status, TF_UNIMPLEMENTED,
                 "Lists of tensors are not yet implemented for default valued "
                 "attributes for an operation.");
  }
}

bool SetOpAttrScalar(
    TFE_Context* ctx, TFE_Op* op, const char* key, PyObject* py_value,
    TF_AttrType type,
    tensorflow::gtl::FlatMap<string, tensorflow::int64>* attr_list_sizes,
    TF_Status* status) {
  if (type == TF_ATTR_STRING) {
    const char* value;
    if (!ParseStringValue(key, py_value, status, &value)) return false;
    TFE_OpSetAttrString(op, key, value);
  } else if (type == TF_ATTR_INT) {
    int64_t value;
    if (!ParseInt64Value(key, py_value, status, &value)) return false;
    TFE_OpSetAttrInt(op, key, value);
    // attr_list_sizes is set for all int attributes (since at this point we are
    // not aware if that attribute might be used to calculate the size of an
    // output list or not).
    if (attr_list_sizes != nullptr) (*attr_list_sizes)[key] = value;
  } else if (type == TF_ATTR_FLOAT) {
    float value;
    if (!ParseFloatValue(key, py_value, status, &value)) return false;
    TFE_OpSetAttrFloat(op, key, value);
  } else if (type == TF_ATTR_BOOL) {
    unsigned char value;
    if (!ParseBoolValue(key, py_value, status, &value)) return false;
    TFE_OpSetAttrBool(op, key, value);
  } else if (type == TF_ATTR_TYPE) {
    int value;
    if (!ParseTypeValue(key, py_value, status, &value)) return false;
    TFE_OpSetAttrType(op, key, static_cast<TF_DataType>(value));
  } else if (type == TF_ATTR_SHAPE) {
    if (py_value == Py_None) {
      TFE_OpSetAttrShape(op, key, nullptr, -1, status);
    } else {
      if (!PySequence_Check(py_value)) {
        TF_SetStatus(status, TF_INVALID_ARGUMENT,
                     tensorflow::strings::StrCat(
                         "Expecting None or sequence value for attr", key,
                         ", got ", py_value->ob_type->tp_name)
                         .c_str());
        return false;
      }
      const auto num_dims = TensorShapeNumDims(py_value);
      if (num_dims == -1) {
        TFE_OpSetAttrShape(op, key, nullptr, -1, status);
        return true;
      }
      std::unique_ptr<int64_t[]> dims(new int64_t[num_dims]);
      for (int i = 0; i < num_dims; ++i) {
        auto inner_py_value = PySequence_ITEM(py_value, i);
        if (inner_py_value == Py_None) {
          dims[i] = -1;
        } else if (!ParseInt64Value(key, inner_py_value, status, &dims[i])) {
          return false;
        }
      }
      TFE_OpSetAttrShape(op, key, dims.get(), num_dims, status);
    }
    if (TF_GetCode(status) != TF_OK) return false;
  } else if (type == TF_ATTR_FUNC) {
    // Allow:
    // (1) String function name, OR
    // (2) A Python object with a .name attribute
    //     (A crude test for being a
    //     tensorflow.python.framework.function._DefinedFunction)
    //     (which is what the various "defun" or "Defun" decorators do).
    // And in the future also allow an object that can encapsulate
    // the function name and its attribute values.
    const char* func_name = nullptr;
    if (!ParseStringValue(key, py_value, status, &func_name)) {
      PyObject* name_attr = PyObject_GetAttrString(py_value, "name");
      if (name_attr == nullptr ||
          !ParseStringValue(key, name_attr, status, &func_name)) {
        TF_SetStatus(
            status, TF_INVALID_ARGUMENT,
            tensorflow::strings::StrCat(
                "unable to set function value attribute from a ",
                py_value->ob_type->tp_name,
                " object. If you think this is an error, please file an issue "
                "at https://github.com/tensorflow/tensorflow/issues/new")
                .c_str());
        return false;
      }
    }
    TFE_Op* func = TFE_NewOp(ctx, func_name, status);
    if (TF_GetCode(status) != TF_OK) return false;
    TFE_OpSetAttrFunction(op, key, func);
    TFE_DeleteOp(func);
  } else {
    TF_SetStatus(
        status, TF_UNIMPLEMENTED,
        tensorflow::strings::StrCat("Attr ", key, " has unhandled type ", type)
            .c_str());
    return false;
  }
  return true;
}

void SetOpAttrScalarDefault(
    TFE_Context* ctx, TFE_Op* op, const tensorflow::AttrValue& default_value,
    const char* attr_name,
    tensorflow::gtl::FlatMap<string, tensorflow::int64>* attr_list_sizes,
    TF_Status* status) {
  switch (default_value.value_case()) {
    case tensorflow::AttrValue::kS:
      TFE_OpSetAttrString(op, attr_name, default_value.s().data());
      break;
    case tensorflow::AttrValue::kI:
      TFE_OpSetAttrInt(op, attr_name, static_cast<int64_t>(default_value.i()));
      (*attr_list_sizes)[attr_name] = default_value.i();
      break;
    case tensorflow::AttrValue::kF:
      TFE_OpSetAttrFloat(op, attr_name, default_value.f());
      break;
    case tensorflow::AttrValue::kB:
      TFE_OpSetAttrBool(op, attr_name, default_value.b());
      break;
    case tensorflow::AttrValue::kType:
      TFE_OpSetAttrType(op, attr_name,
                        static_cast<TF_DataType>(default_value.type()));
      break;
    case tensorflow::AttrValue::kShape: {
      const auto& tensor_shape = default_value.shape();
      if (tensor_shape.unknown_rank()) {
        TFE_OpSetAttrShape(op, attr_name, nullptr, -1, status);
      } else {
        const auto num_dims = tensor_shape.dim_size();
        std::unique_ptr<int64_t[]> dims(new int64_t[num_dims]);
        for (int i = 0; i < num_dims; ++i) {
          dims[i] = tensor_shape.dim(i).size();
        }
        TFE_OpSetAttrShape(op, attr_name, dims.get(), num_dims, status);
      }
    } break;
    case tensorflow::AttrValue::kFunc: {
      const auto func_op = GetFunc(ctx, default_value.func(), status);
      if (TF_GetCode(status) != TF_OK) return;
      // TODO(nareshmodi): TFE_OpSetAttrFunction and TFE_OpSetAttrFunctionList
      // require TFE_Op* and just convert it internally a NameAttrValue, so
      // consider adding an overload to the C API to make this case easier.
      TFE_OpSetAttrFunction(op, attr_name, func_op);
    } break;
    case tensorflow::AttrValue::kList:
      TF_FALLTHROUGH_INTENDED;
    case tensorflow::AttrValue::kTensor:
      TF_FALLTHROUGH_INTENDED;
    case tensorflow::AttrValue::kPlaceholder:
      TF_FALLTHROUGH_INTENDED;
    case tensorflow::AttrValue::VALUE_NOT_SET:
      TF_SetStatus(
          status, TF_UNIMPLEMENTED,
          tensorflow::strings::StrCat("Unable to get setfor default value: ",
                                      default_value.DebugString())
              .data());
  }
}

// start_index is the index at which the Tuple/List attrs will start getting
// processed.
void SetOpAttrs(TFE_Context* ctx, TFE_Op* op, PyObject* attrs, int start_index,
                TF_Status* out_status) {
  if (attrs == Py_None) return;
  Py_ssize_t len = PyTuple_GET_SIZE(attrs) - start_index;
  if ((len & 1) != 0) {
    TF_SetStatus(out_status, TF_INVALID_ARGUMENT,
                 "Expecting attrs tuple to have even length.");
    return;
  }
  // Parse attrs
  for (Py_ssize_t i = 0; i < len; i += 2) {
    PyObject* py_key = PyTuple_GET_ITEM(attrs, start_index + i);
    PyObject* py_value = PyTuple_GET_ITEM(attrs, start_index + i + 1);
#if PY_MAJOR_VERSION >= 3
    const char* key = PyBytes_Check(py_key) ? PyBytes_AsString(py_key)
                                            : PyUnicode_AsUTF8(py_key);
#else
    const char* key = PyBytes_AsString(py_key);
#endif
    unsigned char is_list = 0;
    const TF_AttrType type = TFE_OpGetAttrType(op, key, &is_list, out_status);
    if (TF_GetCode(out_status) != TF_OK) return;
    if (is_list != 0) {
      if (!SetOpAttrList(op, key, py_value, type, nullptr, out_status)) return;
    } else {
      if (!SetOpAttrScalar(ctx, op, key, py_value, type, nullptr, out_status))
        return;
    }
  }
}

// This function will set the op attrs required. If an attr has the value of
// None, then it will read the AttrDef to get the default value and set that
// instead. Any failure in this function will simply fall back to the slow
// path.
void SetOpAttrWithDefaults(
    TFE_Context* ctx, TFE_Op* op, const tensorflow::OpDef::AttrDef& attr,
    const char* attr_name, PyObject* attr_value,
    tensorflow::gtl::FlatMap<string, tensorflow::int64>* attr_list_sizes,
    TF_Status* status) {
  unsigned char is_list = 0;
  const TF_AttrType type = TFE_OpGetAttrType(op, attr_name, &is_list, status);
  if (TF_GetCode(status) != TF_OK) return;
  if (attr_value == Py_None) {
    if (is_list != 0) {
      SetOpAttrListDefault(ctx, op, attr, attr_name, type, attr_list_sizes,
                           status);
    } else {
      SetOpAttrScalarDefault(ctx, op, attr.default_value(), attr_name,
                             attr_list_sizes, status);
    }
  } else {
    if (is_list != 0) {
      SetOpAttrList(op, attr_name, attr_value, type, attr_list_sizes, status);
    } else {
      SetOpAttrScalar(ctx, op, attr_name, attr_value, type, attr_list_sizes,
                      status);
    }
  }
}

// Python subclass of Exception that is created on not ok Status.
tensorflow::mutex exception_class_mutex(tensorflow::LINKER_INITIALIZED);
PyObject* exception_class GUARDED_BY(exception_class_mutex) = nullptr;

// Python subclass of Exception that is created to signal fallback.
PyObject* fallback_exception_class = nullptr;

tensorflow::mutex _uid_mutex(tensorflow::LINKER_INITIALIZED);
tensorflow::int64 _uid GUARDED_BY(_uid_mutex) = 0;

}  // namespace

void TFE_Py_Execute(TFE_Context* ctx, const char* device_name,
                    const char* op_name, TFE_InputTensorHandles* inputs,
                    PyObject* attrs, TFE_OutputTensorHandles* outputs,
                    TF_Status* out_status) {
  TFE_Op* op = TFE_NewOp(ctx, op_name, out_status);
  if (TF_GetCode(out_status) != TF_OK) return;
  TFE_OpSetDevice(op, device_name, out_status);
  if (TF_GetCode(out_status) == TF_OK) {
    for (int i = 0; i < inputs->size() && TF_GetCode(out_status) == TF_OK;
         ++i) {
      TFE_OpAddInput(op, inputs->at(i), out_status);
    }
  }
  if (TF_GetCode(out_status) == TF_OK) {
    SetOpAttrs(ctx, op, attrs, 0, out_status);
  }
  Py_BEGIN_ALLOW_THREADS;
  if (TF_GetCode(out_status) == TF_OK) {
    int num_outputs = outputs->size();
    TFE_Execute(op, outputs->data(), &num_outputs, out_status);
    outputs->resize(num_outputs);
  }
  if (TF_GetCode(out_status) != TF_OK) {
    TF_SetStatus(out_status, TF_GetCode(out_status),
                 tensorflow::strings::StrCat(TF_Message(out_status),
                                             " [Op:", op_name, "]")
                     .c_str());
  }
  TFE_DeleteOp(op);
  Py_END_ALLOW_THREADS;
}

PyObject* TFE_Py_RegisterExceptionClass(PyObject* e) {
  tensorflow::mutex_lock l(exception_class_mutex);
  if (exception_class != nullptr) {
    Py_DECREF(exception_class);
  }
  if (PyObject_IsSubclass(e, PyExc_Exception) <= 0) {
    exception_class = nullptr;
    PyErr_SetString(PyExc_TypeError,
                    "TFE_Py_RegisterExceptionClass: "
                    "Registered class should be subclass of Exception.");
    return nullptr;
  } else {
    Py_INCREF(e);
    exception_class = e;
    Py_RETURN_NONE;
  }
}

PyObject* TFE_Py_RegisterFallbackExceptionClass(PyObject* e) {
  if (fallback_exception_class != nullptr) {
    Py_DECREF(fallback_exception_class);
  }
  if (PyObject_IsSubclass(e, PyExc_Exception) <= 0) {
    fallback_exception_class = nullptr;
    PyErr_SetString(PyExc_TypeError,
                    "TFE_Py_RegisterFallbackExceptionClass: "
                    "Registered class should be subclass of Exception.");
    return nullptr;
  } else {
    Py_INCREF(e);
    fallback_exception_class = e;
    Py_RETURN_NONE;
  }
}

void RaiseFallbackException(const char* message) {
  if (fallback_exception_class != nullptr) {
    PyErr_SetObject(fallback_exception_class, Py_BuildValue("s", message));
    return;
  }

  PyErr_SetString(
      PyExc_RuntimeError,
      tensorflow::strings::StrCat(
          "Fallback exception type not set, attempting to fallback due to ",
          message)
          .data());
}

int MaybeRaiseExceptionFromTFStatus(TF_Status* status, PyObject* exception) {
  if (TF_GetCode(status) == TF_OK) return 0;
  const char* msg = TF_Message(status);
  if (exception == nullptr) {
    tensorflow::mutex_lock l(exception_class_mutex);
    if (exception_class != nullptr) {
      PyErr_SetObject(exception_class,
                      Py_BuildValue("si", msg, TF_GetCode(status)));
      return -1;
    } else {
      exception = PyExc_RuntimeError;
    }
  }
  // May be update already set exception.
  PyErr_SetString(exception, msg);
  return -1;
}

int MaybeRaiseExceptionFromStatus(const tensorflow::Status& status,
                                  PyObject* exception) {
  if (status.ok()) return 0;
  const char* msg = status.error_message().c_str();
  if (exception == nullptr) {
    tensorflow::mutex_lock l(exception_class_mutex);
    if (exception_class != nullptr) {
      PyErr_SetObject(exception_class, Py_BuildValue("si", msg, status.code()));
      return -1;
    } else {
      exception = PyExc_RuntimeError;
    }
  }
  // May be update already set exception.
  PyErr_SetString(exception, msg);
  return -1;
}

char* TFE_GetPythonString(PyObject* o) {
  if (PyBytes_Check(o)) {
    return PyBytes_AsString(o);
  }
#if PY_MAJOR_VERSION >= 3
  if (PyUnicode_Check(o)) {
    return PyUnicode_AsUTF8(o);
  }
#endif
  return nullptr;
}

int64_t get_uid() {
  tensorflow::mutex_lock l(_uid_mutex);
  return _uid++;
}

PyObject* TFE_Py_UID() { return PyLong_FromLongLong(get_uid()); }

void TFE_DeleteContextCapsule(PyObject* context) {
  TF_Status* status = TF_NewStatus();
  TFE_Context* ctx =
      reinterpret_cast<TFE_Context*>(PyCapsule_GetPointer(context, nullptr));
  TFE_DeleteContext(ctx, status);
  TF_DeleteStatus(status);
}

static tensorflow::int64 MakeInt(PyObject* integer) {
#if PY_MAJOR_VERSION >= 3
  return PyLong_AsLong(integer);
#else
  return PyInt_AsLong(integer);
#endif
}

static tensorflow::int64 FastTensorId(PyObject* tensor) {
  if (EagerTensor_CheckExact(tensor)) {
    return EagerTensor_id(tensor);
  }
  PyObject* id_field = PyObject_GetAttrString(tensor, "_id");
  if (id_field == nullptr) {
    return -1;
  }
  tensorflow::int64 id = MakeInt(id_field);
  Py_DECREF(id_field);
  return id;
}

class GradientTape
    : public tensorflow::eager::GradientTape<PyObject, PyObject> {
 public:
  explicit GradientTape(bool persistent)
      : tensorflow::eager::GradientTape<PyObject, PyObject>(persistent) {}

  virtual ~GradientTape() {
    for (PyObject* v : watched_variables_) {
      Py_DECREF(v);
    }
  }

  void WatchVariable(PyObject* v) {
    auto insert_result = watched_variables_.insert(v);
    if (insert_result.second) {
      // Only increment the reference count if we aren't already watching this
      // variable.
      Py_INCREF(v);
    }
    PyObject* handle = PyObject_GetAttrString(v, "handle");
    if (handle == nullptr) {
      return;
    }
    tensorflow::int64 id = FastTensorId(handle);
    Py_DECREF(handle);
    if (!PyErr_Occurred()) {
      this->Watch(id);
    }
  }

  const std::unordered_set<PyObject*> WatchedVariables() {
    return watched_variables_;
  }

 private:
  std::unordered_set<PyObject*> watched_variables_;
};

typedef struct {
  PyObject_HEAD
      /* Type-specific fields go here. */
      GradientTape* tape;
} TFE_Py_Tape;

static void TFE_Py_Tape_Delete(PyObject* tape) {
  delete reinterpret_cast<TFE_Py_Tape*>(tape)->tape;
  Py_TYPE(tape)->tp_free(tape);
}

static PyTypeObject TFE_Py_Tape_Type = {
    PyVarObject_HEAD_INIT(nullptr, 0) "tfe.Tape", /* tp_name */
    sizeof(TFE_Py_Tape),                          /* tp_basicsize */
    0,                                            /* tp_itemsize */
    &TFE_Py_Tape_Delete,                          /* tp_dealloc */
    nullptr,                                      /* tp_print */
    nullptr,                                      /* tp_getattr */
    nullptr,                                      /* tp_setattr */
    nullptr,                                      /* tp_reserved */
    nullptr,                                      /* tp_repr */
    nullptr,                                      /* tp_as_number */
    nullptr,                                      /* tp_as_sequence */
    nullptr,                                      /* tp_as_mapping */
    nullptr,                                      /* tp_hash  */
    nullptr,                                      /* tp_call */
    nullptr,                                      /* tp_str */
    nullptr,                                      /* tp_getattro */
    nullptr,                                      /* tp_setattro */
    nullptr,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                           /* tp_flags */
    "TFE_Py_Tape objects",                        /* tp_doc */
};

// Note: in the current design no mutex is needed here because of the python
// GIL, which is always held when any TFE_Py_* methods are called. We should
// revisit this if/when decide to not hold the GIL while manipulating the tape
// stack.
static tensorflow::gtl::CompactPointerSet<TFE_Py_Tape*>* tape_set = nullptr;
tensorflow::gtl::CompactPointerSet<TFE_Py_Tape*>* GetTapeSet() {
  if (tape_set == nullptr) {
    tape_set = new tensorflow::gtl::CompactPointerSet<TFE_Py_Tape*>;
  }
  return tape_set;
}

// A safe copy of the current tapeset. Does not get affected by other python
// threads changing the set of active tapes.
class SafeTapeSet {
 public:
  SafeTapeSet() : tape_set_(*GetTapeSet()) {
    for (auto* tape : tape_set_) {
      Py_INCREF(tape);
    }
  }

  ~SafeTapeSet() {
    for (auto* tape : tape_set_) {
      Py_DECREF(tape);
    }
  }

  tensorflow::gtl::CompactPointerSet<TFE_Py_Tape*>::const_iterator begin() {
    return tape_set_.begin();
  }

  tensorflow::gtl::CompactPointerSet<TFE_Py_Tape*>::const_iterator end() {
    return tape_set_.end();
  }

 private:
  tensorflow::gtl::CompactPointerSet<TFE_Py_Tape*> tape_set_;
};

// xcode 7 doesn't define thread_local, so for compatibility we implement our
// own. TODO(apassos) remove once we can deprecate xcode 7.
#ifndef __APPLE__
bool* ThreadTapeIsStopped() {
  thread_local bool thread_tape_is_stopped{false};
  return &thread_tape_is_stopped;
}
#else
static std::unordered_map<std::thread::id, bool>* tape_is_stopped = nullptr;
bool* ThreadTapeIsStopped() {
  if (tape_is_stopped == nullptr) {
    tape_is_stopped = new std::unordered_map<std::thread::id, bool>;
  }
  auto it = tape_is_stopped->find(std::this_thread::get_id());
  if (it != tape_is_stopped->end()) {
    return &(it->second);
  }
  return &(tape_is_stopped->emplace(std::this_thread::get_id(), false)
               .first->second);
}
#endif

void TFE_Py_TapeSetStopOnThread() { *ThreadTapeIsStopped() = true; }

void TFE_Py_TapeSetRestartOnThread() { *ThreadTapeIsStopped() = false; }

PyObject* TFE_Py_TapeSetNew(PyObject* persistent) {
  TFE_Py_Tape_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&TFE_Py_Tape_Type) < 0) return nullptr;
  TFE_Py_Tape* tape = PyObject_NEW(TFE_Py_Tape, &TFE_Py_Tape_Type);
  tape->tape = new GradientTape(persistent == Py_True);
  Py_INCREF(tape);
  GetTapeSet()->insert(reinterpret_cast<TFE_Py_Tape*>(tape));
  return reinterpret_cast<PyObject*>(tape);
}

PyObject* TFE_Py_TapeSetIsEmpty() {
  if (*ThreadTapeIsStopped() || GetTapeSet()->empty()) {
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

void TFE_Py_TapeSetRemove(PyObject* tape) {
  auto* stack = GetTapeSet();
  stack->erase(reinterpret_cast<TFE_Py_Tape*>(tape));
  // We kept a reference to the tape in the set to ensure it wouldn't get
  // deleted under us; cleaning it up here.
  Py_DECREF(tape);
}

static std::vector<tensorflow::int64> MakeIntList(PyObject* list) {
  if (list == Py_None) {
    return {};
  }
  PyObject* seq = PySequence_Fast(list, "expected a sequence");
  if (seq == nullptr) {
    return {};
  }
  int len = PySequence_Size(list);
  std::vector<tensorflow::int64> tensor_ids;
  tensor_ids.reserve(len);
  for (int i = 0; i < len; ++i) {
    PyObject* item = PySequence_Fast_GET_ITEM(seq, i);
#if PY_MAJOR_VERSION >= 3
    if (PyLong_Check(item)) {
#else
    if (PyLong_Check(item) || PyInt_Check(item)) {
#endif
      tensorflow::int64 id = MakeInt(item);
      tensor_ids.push_back(id);
    } else {
      tensor_ids.push_back(-1);
    }
  }
  Py_DECREF(seq);
  return tensor_ids;
}

PyObject* TFE_Py_TapeSetShouldRecord(PyObject* tensors) {
  if (tensors == Py_None) {
    Py_RETURN_FALSE;
  }
  if (*ThreadTapeIsStopped()) {
    Py_RETURN_FALSE;
  }
  auto* tape_set_ptr = GetTapeSet();
  if (tape_set_ptr->empty()) {
    Py_RETURN_FALSE;
  }
  PyObject* seq = PySequence_Fast(tensors, "expected a sequence");
  if (seq == nullptr) {
    return nullptr;
  }
  int len = PySequence_Fast_GET_SIZE(seq);
  // TODO(apassos) consider not building a list and changing the API to check
  // each tensor individually.
  std::vector<tensorflow::int64> tensor_ids;
  tensor_ids.reserve(len);
  for (int i = 0; i < len; ++i) {
    PyObject* item = PySequence_Fast_GET_ITEM(seq, i);
    tensor_ids.push_back(FastTensorId(item));
  }
  Py_DECREF(seq);
  auto tape_set = *tape_set_ptr;
  for (TFE_Py_Tape* tape : tape_set) {
    if (tape->tape->ShouldRecord(tensor_ids)) {
      Py_RETURN_TRUE;
    }
  }
  Py_RETURN_FALSE;
}

void TFE_Py_TapeSetWatch(PyObject* tensor) {
  if (*ThreadTapeIsStopped()) {
    return;
  }
  tensorflow::int64 tensor_id = FastTensorId(tensor);
  if (PyErr_Occurred()) {
    return;
  }
  for (TFE_Py_Tape* tape : *GetTapeSet()) {
    tape->tape->Watch(tensor_id);
  }
}

static tensorflow::eager::TapeTensor TapeTensorFromTensor(PyObject* tensor) {
  if (EagerTensor_CheckExact(tensor)) {
    TFE_TensorHandle* t = EagerTensor_Handle(tensor);
    tensorflow::int64 id = EagerTensor_id(tensor);
    return tensorflow::eager::TapeTensor{id, t->t.dtype(), t->t.shape()};
  }
  tensorflow::int64 id = FastTensorId(tensor);
  if (PyErr_Occurred()) {
    return tensorflow::eager::TapeTensor{
        id, static_cast<tensorflow::DataType>(0), tensorflow::TensorShape({})};
  }
  PyObject* dtype_object = PyObject_GetAttrString(tensor, "dtype");
  PyObject* dtype_enum = PyObject_GetAttrString(dtype_object, "_type_enum");
  Py_DECREF(dtype_object);
  tensorflow::DataType dtype =
      static_cast<tensorflow::DataType>(MakeInt(dtype_enum));
  Py_DECREF(dtype_enum);
  if (PyErr_Occurred() != nullptr) {
    return tensorflow::eager::TapeTensor{id, dtype,
                                         tensorflow::TensorShape({})};
  }
  static char _shape_tuple[] = "_shape_tuple";
  PyObject* shape_tuple = PyObject_CallMethod(tensor, _shape_tuple, nullptr);
  if (PyErr_Occurred() != nullptr) {
    return tensorflow::eager::TapeTensor{id, dtype,
                                         tensorflow::TensorShape({})};
  }
  auto l = MakeIntList(shape_tuple);
  Py_DECREF(shape_tuple);
  // Replace -1, which represents accidental Nones which can occur in graph mode
  // and can cause errors in shape cosntruction with 0s.
  for (auto& c : l) {
    if (c < 0) {
      c = 0;
    }
  }
  tensorflow::TensorShape shape(l);
  return tensorflow::eager::TapeTensor{id, dtype, shape};
}

std::vector<tensorflow::int64> MakeTensorIDList(PyObject* tensors) {
  PyObject* seq = PySequence_Fast(tensors, "expected a sequence");
  if (seq == nullptr) {
    return {};
  }
  int len = PySequence_Fast_GET_SIZE(seq);
  std::vector<tensorflow::int64> list;
  list.reserve(len);
  for (int i = 0; i < len; ++i) {
    PyObject* tensor = PySequence_Fast_GET_ITEM(seq, i);
    list.push_back(FastTensorId(tensor));
    if (PyErr_Occurred()) {
      Py_DECREF(seq);
      return list;
    }
  }
  Py_DECREF(seq);
  return list;
}

void TFE_Py_TapeSetWatchVariable(PyObject* variable) {
  if (*ThreadTapeIsStopped()) {
    return;
  }
  for (TFE_Py_Tape* tape : SafeTapeSet()) {
    tape->tape->WatchVariable(variable);
  }
}

PyObject* TFE_Py_TapeWatchedVariables(PyObject* tape) {
  const std::unordered_set<PyObject*>& watched_variables =
      reinterpret_cast<TFE_Py_Tape*>(tape)->tape->WatchedVariables();
  PyObject* result = PySet_New(nullptr);
  for (PyObject* variable : watched_variables) {
    PySet_Add(result, variable);
  }
  return result;
}

void TFE_Py_TapeSetRecordOperation(PyObject* op_type, PyObject* output_tensors,
                                   PyObject* input_tensors,
                                   PyObject* backward_function) {
  if (GetTapeSet()->empty() || *ThreadTapeIsStopped()) {
    return;
  }
  std::vector<tensorflow::int64> input_ids = MakeTensorIDList(input_tensors);
  if (PyErr_Occurred()) {
    return;
  }
  std::vector<tensorflow::eager::TapeTensor> output_info;
  PyObject* seq = PySequence_Fast(output_tensors,
                                  "expected a sequence of integer tensor ids");
  int len = PySequence_Size(output_tensors);
  output_info.reserve(len);
  for (int i = 0; i < len; ++i) {
    output_info.push_back(
        TapeTensorFromTensor(PySequence_Fast_GET_ITEM(seq, i)));
    if (PyErr_Occurred() != nullptr) {
      Py_DECREF(seq);
      return;
    }
  }
  Py_DECREF(seq);
  string op_type_str;
  if (PyBytes_Check(op_type)) {
    op_type_str = PyBytes_AsString(op_type);
  } else if (PyUnicode_Check(op_type)) {
#if PY_MAJOR_VERSION >= 3
    op_type_str = PyUnicode_AsUTF8(op_type);
#else
    PyObject* py_str = PyUnicode_AsUTF8String(op_type);
    if (py_str == nullptr) return;
    op_type_str = PyBytes_AS_STRING(py_str);
    Py_DECREF(py_str);
#endif
  } else {
    PyErr_SetString(PyExc_RuntimeError, "op_type should be a string.");
    return;
  }

  for (TFE_Py_Tape* tape : SafeTapeSet()) {
    Py_INCREF(backward_function);
    tape->tape->RecordOperation(
        op_type_str, output_info, input_ids, backward_function,
        [backward_function]() { Py_DECREF(backward_function); });
  }
}

void TFE_Py_TapeSetDeleteTrace(tensorflow::int64 tensor_id) {
  for (TFE_Py_Tape* tape : SafeTapeSet()) {
    tape->tape->DeleteTrace(tensor_id);
  }
}

class PyVSpace : public tensorflow::eager::VSpace<PyObject, PyObject> {
 public:
  explicit PyVSpace(PyObject* py_vspace) : py_vspace_(py_vspace) {}

  tensorflow::Status Initialize() {
    num_elements_ = PyObject_GetAttrString(py_vspace_, "num_elements_fn");
    if (num_elements_ == nullptr) {
      return tensorflow::errors::InvalidArgument("invalid vspace");
    }
    aggregate_fn_ = PyObject_GetAttrString(py_vspace_, "aggregate_fn");
    if (aggregate_fn_ == nullptr) {
      return tensorflow::errors::InvalidArgument("invalid vspace");
    }
    zeros_ = PyObject_GetAttrString(py_vspace_, "zeros");
    if (zeros_ == nullptr) {
      return tensorflow::errors::InvalidArgument("invalid vspace");
    }
    ones_ =
        PyObject_GetAttrString(reinterpret_cast<PyObject*>(py_vspace_), "ones");
    if (ones_ == nullptr) {
      return tensorflow::errors::InvalidArgument("invalid vspace");
    }
    return tensorflow::Status::OK();
  }

  ~PyVSpace() override {
    Py_XDECREF(num_elements_);
    Py_XDECREF(aggregate_fn_);
    Py_XDECREF(zeros_);
    Py_XDECREF(ones_);
  }

  tensorflow::int64 NumElements(PyObject* tensor) const final {
    PyObject* arglist =
        Py_BuildValue("(O)", reinterpret_cast<PyObject*>(tensor));
    PyObject* result = PyEval_CallObject(num_elements_, arglist);
    tensorflow::int64 r = MakeInt(result);
    Py_DECREF(result);
    Py_DECREF(arglist);
    return r;
  }

  PyObject* AggregateGradients(
      tensorflow::gtl::ArraySlice<PyObject*> gradient_tensors) const final {
    PyObject* list = PyList_New(gradient_tensors.size());
    for (int i = 0; i < gradient_tensors.size(); ++i) {
      // Note: stealing a reference to the gradient tensors.
      CHECK(gradient_tensors[i] != nullptr);
      CHECK(gradient_tensors[i] != Py_None);
      PyList_SET_ITEM(list, i,
                      reinterpret_cast<PyObject*>(gradient_tensors[i]));
    }
    PyObject* arglist = Py_BuildValue("(O)", list);
    CHECK(arglist != nullptr);
    PyObject* result = PyEval_CallObject(aggregate_fn_, arglist);
    Py_DECREF(arglist);
    Py_DECREF(list);
    return result;
  }

  PyObject* Zeros(tensorflow::TensorShape shape,
                  tensorflow::DataType dtype) const final {
    PyObject* py_shape = PyTuple_New(shape.dims());
    for (int i = 0; i < shape.dims(); ++i) {
      PyTuple_SET_ITEM(py_shape, i, PyLong_FromLong(shape.dim_size(i)));
    }
    PyObject* py_dtype = PyLong_FromLong(static_cast<int>(dtype));
    PyObject* arg_list = Py_BuildValue("OO", py_shape, py_dtype);
    PyObject* result = PyEval_CallObject(zeros_, arg_list);
    Py_DECREF(arg_list);
    Py_DECREF(py_dtype);
    Py_DECREF(py_shape);
    return reinterpret_cast<PyObject*>(result);
  }

  PyObject* Ones(tensorflow::TensorShape shape,
                 tensorflow::DataType dtype) const final {
    PyObject* py_shape = PyTuple_New(shape.dims());
    for (int i = 0; i < shape.dims(); ++i) {
      PyTuple_SET_ITEM(py_shape, i, PyLong_FromLong(shape.dim_size(i)));
    }
    PyObject* py_dtype = PyLong_FromLong(static_cast<int>(dtype));
    PyObject* arg_list = Py_BuildValue("OO", py_shape, py_dtype);
    PyObject* result = PyEval_CallObject(ones_, arg_list);
    Py_DECREF(arg_list);
    Py_DECREF(py_dtype);
    Py_DECREF(py_shape);
    return result;
  }

  tensorflow::Status CallBackwardFunction(
      PyObject* backward_function,
      tensorflow::gtl::ArraySlice<PyObject*> output_gradients,
      std::vector<PyObject*>* result) const final {
    PyObject* grads = PyTuple_New(output_gradients.size());
    for (int i = 0; i < output_gradients.size(); ++i) {
      if (output_gradients[i] == nullptr) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(grads, i, Py_None);
      } else {
        PyTuple_SET_ITEM(grads, i,
                         reinterpret_cast<PyObject*>(output_gradients[i]));
      }
    }
    PyObject* py_result = PyEval_CallObject(
        reinterpret_cast<PyObject*>(backward_function), grads);
    Py_DECREF(grads);
    if (py_result == nullptr) {
      return tensorflow::errors::Internal("gradient function threw exceptions");
    }
    result->clear();
    PyObject* seq =
        PySequence_Fast(py_result, "expected a sequence of gradients");
    if (seq == nullptr) {
      return tensorflow::errors::InvalidArgument(
          "gradient function did not return a list");
    }
    int len = PySequence_Fast_GET_SIZE(seq);
    VLOG(1) << "Gradient length is " << len;
    result->reserve(len);
    for (int i = 0; i < len; ++i) {
      PyObject* item = PySequence_Fast_GET_ITEM(seq, i);
      if (item == Py_None) {
        result->push_back(nullptr);
      } else {
        Py_INCREF(item);
        result->push_back(item);
      }
    }
    Py_DECREF(seq);
    Py_DECREF(py_result);
    return tensorflow::Status::OK();
  }

  void ReleaseBackwardFunction(PyObject* backward_function) const final {
    Py_DECREF(backward_function);
  }

  void DeleteGradient(PyObject* tensor) const final { Py_XDECREF(tensor); }

 private:
  PyObject* py_vspace_;

  PyObject* num_elements_;
  PyObject* aggregate_fn_;
  PyObject* zeros_;
  PyObject* ones_;
};

std::vector<PyObject*> MakeTensorList(PyObject* tensors) {
  PyObject* seq = PySequence_Fast(tensors, "expected a sequence");
  if (seq == nullptr) {
    return {};
  }
  int len = PySequence_Fast_GET_SIZE(seq);
  std::vector<PyObject*> list;
  list.reserve(len);
  for (int i = 0; i < len; ++i) {
    list.push_back(PySequence_Fast_GET_ITEM(seq, i));
  }
  Py_DECREF(seq);
  return list;
}

PyObject* TFE_Py_TapeGradient(PyObject* tape, PyObject* vspace,
                              PyObject* target, PyObject* sources,
                              PyObject* output_gradients, TF_Status* status) {
  PyVSpace c_vspace(vspace);
  if (!c_vspace.Initialize().ok()) {
    return nullptr;
  }

  std::vector<tensorflow::int64> target_vec = MakeTensorIDList(target);
  if (PyErr_Occurred()) {
    return nullptr;
  }
  std::vector<tensorflow::int64> sources_vec = MakeTensorIDList(sources);
  if (PyErr_Occurred()) {
    return nullptr;
  }
  std::vector<PyObject*> outgrad_vec;
  if (output_gradients != Py_None) {
    outgrad_vec = MakeTensorList(output_gradients);
    if (PyErr_Occurred()) {
      return nullptr;
    }
    for (PyObject* tensor : outgrad_vec) {
      // Calling the backward function will eat a reference to the tensors in
      // outgrad_vec, so we need to increase their reference count.
      Py_INCREF(tensor);
    }
  }
  TFE_Py_Tape* tape_obj = reinterpret_cast<TFE_Py_Tape*>(tape);
  std::vector<PyObject*> result;
  status->status = tape_obj->tape->ComputeGradient(
      c_vspace, target_vec, sources_vec, outgrad_vec, &result);
  if (!status->status.ok()) {
    if (PyErr_Occurred()) {
      // Do not propagate the erroneous status as that would swallow the
      // exception which caused the problem.
      status->status = tensorflow::Status::OK();
    }
    return nullptr;
  }
  if (!result.empty()) {
    PyObject* py_result = PyList_New(result.size());
    for (int i = 0; i < result.size(); ++i) {
      if (result[i] == nullptr) {
        Py_INCREF(Py_None);
        result[i] = Py_None;
      }
      PyList_SET_ITEM(py_result, i, reinterpret_cast<PyObject*>(result[i]));
    }
    return py_result;
  }
  return PyList_New(0);
}

namespace {
static const int kFastPathExecuteInputStartIndex = 6;

PyObject* GetPythonObjectFromString(const char* s) {
#if PY_MAJOR_VERSION >= 3
  return PyUnicode_FromString(s);
#else
  return PyBytes_FromString(s);
#endif
}

bool CheckEagerTensors(PyObject* seq, int start_index,
                       const tensorflow::OpDef& op_def) {
  for (int i = 0; i < op_def.input_arg_size(); i++) {
    PyObject* item = PyTuple_GET_ITEM(seq, i + start_index);
    if (!op_def.input_arg(i).number_attr().empty() ||
        !op_def.input_arg(i).type_list_attr().empty()) {
      // This item should be a list input.
      if (!PyList_Check(item)) return false;
      for (Py_ssize_t j = 0; j < PyList_Size(item); j++) {
        if (!EagerTensor_CheckExact(PyList_GET_ITEM(item, j))) return false;
      }
    } else if (!EagerTensor_CheckExact(item)) {
      return false;
    }
  }

  return true;
}

// Adds input and type attr to the op, and to the list of flattened
// inputs/attrs.
bool AddInputToOp(PyObject* input, const tensorflow::OpDef::ArgDef* input_arg,
                  std::vector<PyObject*>* flattened_attrs,
                  std::vector<PyObject*>* flattened_inputs, TFE_Op* op,
                  TF_Status* status) {
  TFE_TensorHandle* input_handle = EagerTensor_Handle(input);
  if (input_arg != nullptr && !input_arg->type_attr().empty()) {
    auto dtype = TFE_TensorHandleDataType(input_handle);
    TFE_OpSetAttrType(op, input_arg->type_attr().data(), dtype);
    if (flattened_attrs != nullptr) {
      flattened_attrs->push_back(
          GetPythonObjectFromString(input_arg->type_attr().data()));
      flattened_attrs->push_back(PyLong_FromLong(dtype));
    }
  }

  if (flattened_inputs != nullptr) {
    flattened_inputs->push_back(input);
  }
  TFE_OpAddInput(op, input_handle, status);
  if (MaybeRaiseExceptionFromTFStatus(status, nullptr)) {
    return false;
  }
  return true;
}

const tensorflow::OpDef* GetOpDef(PyObject* py_op_name) {
  const char* op_name = TFE_GetPythonString(py_op_name);
  if (op_name == nullptr) {
    PyErr_SetString(PyExc_TypeError,
                    Printf("expected a string for op_name, got %s instead",
                           py_op_name->ob_type->tp_name)
                        .c_str());
    return nullptr;
  }

  const tensorflow::OpRegistrationData* op_reg_data = nullptr;
  const tensorflow::Status lookup_status =
      tensorflow::OpRegistry::Global()->LookUp(op_name, &op_reg_data);
  if (MaybeRaiseExceptionFromStatus(lookup_status, nullptr)) {
    return nullptr;
  }
  return &op_reg_data->op_def;
}

const char* GetDeviceName(PyObject* py_device_name) {
  if (py_device_name != Py_None) {
    return TFE_GetPythonString(py_device_name);
  }
  return nullptr;
}

bool RaiseIfNotPyList(PyObject* list, const string& attr_name) {
  if (!PyList_Check(list)) {
    PyErr_SetString(PyExc_TypeError,
                    Printf("expected a list for attr %s, got %s instead",
                           attr_name.data(), list->ob_type->tp_name)
                        .data());

    return false;
  }
  return true;
}

bool RunCallbacks(bool run_gradient_callback, bool run_post_exec_callbacks,
                  const tensorflow::OpDef* op_def, PyObject* args,
                  const std::vector<PyObject*>& flattened_inputs,
                  const std::vector<PyObject*>& flattened_attrs,
                  PyObject* flattened_result, PyObject* op_name, PyObject* name,
                  PyObject* record_gradient_callback, PyObject* callbacks) {
  PyObject* inputs = PyTuple_New(flattened_inputs.size());
  for (int i = 0; i < flattened_inputs.size(); i++) {
    PyObject* input = flattened_inputs[i];
    Py_INCREF(input);
    PyTuple_SET_ITEM(inputs, i, input);
  }

  int num_non_inferred_attrs = PyTuple_GET_SIZE(args) -
                               op_def->input_arg_size() -
                               kFastPathExecuteInputStartIndex;
  int num_attrs = flattened_attrs.size() + num_non_inferred_attrs;
  PyObject* attrs = PyTuple_New(num_attrs);

  for (int i = 0; i < num_non_inferred_attrs; i++) {
    auto* attr = PyTuple_GET_ITEM(
        args, kFastPathExecuteInputStartIndex + op_def->input_arg_size() + i);
    Py_INCREF(attr);
    PyTuple_SET_ITEM(attrs, i, attr);
  }
  for (int i = num_non_inferred_attrs; i < num_attrs; i++) {
    // Not INCREFing anything in flattened_attrs as each of those is a new
    // reference, so allow the attrs tuple to steal the reference.
    PyTuple_SET_ITEM(attrs, i, flattened_attrs.at(i - num_non_inferred_attrs));
  }

  PyObject* callback_args =
      Py_BuildValue("OOOOO", op_name, inputs, attrs, flattened_result, name);

  auto cleaner = tensorflow::gtl::MakeCleanup([inputs, attrs, callback_args] {
    Py_DECREF(inputs);
    Py_DECREF(attrs);
    Py_DECREF(callback_args);
  });

  if (run_gradient_callback) {
    if (!PyCallable_Check(record_gradient_callback)) {
      PyErr_SetString(PyExc_TypeError,
                      Printf("expected a function for "
                             "record_gradient_callback, got %s instead",
                             record_gradient_callback->ob_type->tp_name)
                          .c_str());
      return false;
    }

    PyObject* callback_result =
        PyObject_CallObject(record_gradient_callback, callback_args);
    if (!callback_result) {
      return false;
    }
    Py_DECREF(callback_result);
  }

  if (run_post_exec_callbacks) {
    for (Py_ssize_t i = 0; i < PyList_Size(callbacks); i++) {
      PyObject* callback_fn = PyList_GET_ITEM(callbacks, i);
      if (!PyCallable_Check(callback_fn)) {
        PyErr_SetString(
            PyExc_TypeError,
            Printf("expected a function for "
                   "post execution callback in index %ld, got %s instead",
                   i, callback_fn->ob_type->tp_name)
                .c_str());
        return false;
      }
      PyObject* callback_result =
          PyObject_CallObject(callback_fn, callback_args);
      if (!callback_result) {
        return false;
      }
      Py_DECREF(callback_result);
    }
  }

  return true;
}

}  // namespace

PyObject* TFE_Py_FastPathExecute_C(PyObject*, PyObject* args) {
  Py_ssize_t args_size = PyTuple_GET_SIZE(args);
  if (args_size < kFastPathExecuteInputStartIndex) {
    PyErr_SetString(
        PyExc_ValueError,
        Printf("There must be at least %d items in the input tuple.",
               kFastPathExecuteInputStartIndex)
            .c_str());
    return nullptr;
  }

  TFE_Context* ctx = reinterpret_cast<TFE_Context*>(
      PyCapsule_GetPointer(PyTuple_GET_ITEM(args, 0), nullptr));
  const char* device_name = GetDeviceName(PyTuple_GET_ITEM(args, 1));
  PyObject* op_name = PyTuple_GET_ITEM(args, 2);
  const tensorflow::OpDef* op_def = GetOpDef(op_name);
  if (op_def == nullptr) return nullptr;
  PyObject* record_gradient_callback = PyTuple_GET_ITEM(args, 3);
  PyObject* name = PyTuple_GET_ITEM(args, 4);
  PyObject* callbacks = PyTuple_GET_ITEM(args, 5);

  if (args_size < kFastPathExecuteInputStartIndex + op_def->input_arg_size()) {
    PyErr_SetString(
        PyExc_ValueError,
        Printf("Tuple size smaller than intended. Expected to be at least %d, "
               "was %ld",
               kFastPathExecuteInputStartIndex + op_def->input_arg_size(),
               args_size)
            .c_str());
    return nullptr;
  }

  if (!CheckEagerTensors(args, kFastPathExecuteInputStartIndex, *op_def)) {
    RaiseFallbackException(
        "This function does not handle the case of the path where "
        "all inputs are not already EagerTensors.");
    return nullptr;
  }

  TF_Status* status = TF_NewStatus();
  TFE_Op* op = TFE_NewOp(ctx, op_def->name().c_str(), status);
  auto cleaner = tensorflow::gtl::MakeCleanup([status, op] {
    TF_DeleteStatus(status);
    TFE_DeleteOp(op);
  });
  if (MaybeRaiseExceptionFromTFStatus(status, nullptr)) {
    return nullptr;
  }

  // Mapping of attr name to size - used to calculate the number of values
  // to be expected by the TFE_Execute run.
  tensorflow::gtl::FlatMap<string, tensorflow::int64> attr_list_sizes;

  // Set non-inferred attrs, including setting defaults if the attr is passed in
  // as None.
  for (int i = kFastPathExecuteInputStartIndex + op_def->input_arg_size();
       i < args_size; i += 2) {
    PyObject* py_attr_name = PyTuple_GET_ITEM(args, i);
    const tensorflow::StringPiece attr_name(TFE_GetPythonString(py_attr_name));
    PyObject* py_attr_value = PyTuple_GET_ITEM(args, i + 1);

    // Not creating an index since most of the time there are not more than a
    // few attrs.
    // TODO(nareshmodi): Maybe include the index as part of the
    // OpRegistrationData.
    for (const auto& attr : op_def->attr()) {
      if (attr_name == attr.name()) {
        SetOpAttrWithDefaults(ctx, op, attr, attr_name.data(), py_attr_value,
                              &attr_list_sizes, status);

        if (TF_GetCode(status) != TF_OK) {
          RaiseFallbackException(TF_Message(status));
          return nullptr;
        }

        break;
      }
    }
  }

  TFE_OpSetDevice(op, device_name, status);
  if (MaybeRaiseExceptionFromTFStatus(status, nullptr)) {
    return nullptr;
  }

  // TODO(nareshmodi): Add a benchmark for the fast-path with gradient callbacks
  // (similar to benchmark_tf_gradient_function_*). Also consider using an
  // InlinedVector for flattened_attrs and flattened_inputs if the benchmarks
  // point out problems with heap allocs.
  bool run_gradient_callback = !*ThreadTapeIsStopped() &&
                               !GetTapeSet()->empty() &&
                               record_gradient_callback != Py_None;
  bool run_post_exec_callbacks =
      callbacks != Py_None && PyList_Size(callbacks) > 0;
  bool run_callbacks = run_gradient_callback || run_post_exec_callbacks;
  // Flat attrs and inputs as required by the record_gradient call. The attrs
  // here only contain inferred attrs (non-inferred attrs are added directly
  // from the input args).
  // All items in flattened_attrs contain new references.
  // All items in flattened_inputs contain borrowed references.
  // TODO(nareshmodi): figure out why PyList_New/PyList_Append don't work
  // directly.
  std::unique_ptr<std::vector<PyObject*>> flattened_attrs = nullptr;
  std::unique_ptr<std::vector<PyObject*>> flattened_inputs = nullptr;

  if (run_callbacks) {
    flattened_attrs.reset(new std::vector<PyObject*>);
    flattened_inputs.reset(new std::vector<PyObject*>);
  }

  // Add inferred attrs and inputs.
  // The following code might set duplicate type attrs. This will result in
  // the CacheKey for the generated AttrBuilder possibly differing from
  // those where the type attrs are correctly set. Inconsistent CacheKeys
  // for ops means that there might be unnecessarily duplicated kernels.
  // TODO(nareshmodi): Fix this.
  for (int i = 0; i < op_def->input_arg_size(); i++) {
    const auto& input_arg = op_def->input_arg(i);

    PyObject* input =
        PyTuple_GET_ITEM(args, kFastPathExecuteInputStartIndex + i);
    if (!input_arg.number_attr().empty()) {
      // The item is a homogeneous list.
      if (!RaiseIfNotPyList(input, input_arg.number_attr())) return nullptr;
      Py_ssize_t len = PyList_Size(input);

      TFE_OpSetAttrInt(op, input_arg.number_attr().data(), len);
      if (run_callbacks) {
        flattened_attrs->push_back(
            GetPythonObjectFromString(input_arg.number_attr().data()));
        flattened_attrs->push_back(PyLong_FromLong(len));
      }
      attr_list_sizes[input_arg.number_attr()] = len;

      if (len > 0) {
        // First item adds the type attr.
        if (!AddInputToOp(PyList_GET_ITEM(input, 0), &input_arg,
                          flattened_attrs.get(), flattened_inputs.get(), op,
                          status)) {
          return nullptr;
        }

        for (Py_ssize_t j = 1; j < len; j++) {
          // Since the list is homogeneous, we don't need to re-add the attr.
          if (!AddInputToOp(PyList_GET_ITEM(input, j), nullptr /* input_arg */,
                            nullptr /* flattened_attrs */,
                            flattened_inputs.get(), op, status)) {
            return nullptr;
          }
        }
      }
    } else if (!input_arg.type_list_attr().empty()) {
      // The item is a heterogeneous list.
      if (!RaiseIfNotPyList(input, input_arg.type_list_attr())) return nullptr;
      const string& attr_name = input_arg.type_list_attr();
      Py_ssize_t len = PyList_Size(input);
      tensorflow::gtl::InlinedVector<TF_DataType, 4> attr_value(len);
      PyObject* py_attr_value = nullptr;
      if (run_callbacks) {
        py_attr_value = PyTuple_New(len);
      }
      for (Py_ssize_t j = 0; j < len; j++) {
        PyObject* py_input = PyList_GET_ITEM(input, j);
        TFE_TensorHandle* input_handle = EagerTensor_Handle(py_input);
        attr_value[j] = TFE_TensorHandleDataType(input_handle);

        TFE_OpAddInput(op, input_handle, status);
        if (MaybeRaiseExceptionFromTFStatus(status, nullptr)) {
          return nullptr;
        }

        if (run_callbacks) {
          flattened_inputs->push_back(py_input);

          PyTuple_SET_ITEM(py_attr_value, j, PyLong_FromLong(attr_value[j]));
        }
      }
      if (run_callbacks) {
        flattened_attrs->push_back(GetPythonObjectFromString(attr_name.data()));
        flattened_attrs->push_back(py_attr_value);
      }
      TFE_OpSetAttrTypeList(op, attr_name.data(), attr_value.data(),
                            attr_value.size());
      attr_list_sizes[attr_name] = len;
    } else {
      // The item is a single item.
      if (!AddInputToOp(input, &input_arg, flattened_attrs.get(),
                        flattened_inputs.get(), op, status)) {
        return nullptr;
      }
    }
  }

  int num_retvals = 0;
  for (int i = 0; i < op_def->output_arg_size(); i++) {
    const auto& output_arg = op_def->output_arg(i);
    if (!output_arg.number_attr().empty()) {
      num_retvals += attr_list_sizes[output_arg.number_attr()];
    } else if (!output_arg.type_list_attr().empty()) {
      num_retvals += attr_list_sizes[output_arg.type_list_attr()];
    } else {
      num_retvals++;
    }
  }

  tensorflow::gtl::InlinedVector<TFE_TensorHandle*, 2> retvals(num_retvals);

  Py_BEGIN_ALLOW_THREADS;
  TFE_Execute(op, retvals.data(), &num_retvals, status);
  Py_END_ALLOW_THREADS;
  if (TF_GetCode(status) != TF_OK) {
    // Augment the status with the op_name for easier debugging similar to
    // TFE_Py_Execute.
    TF_SetStatus(status, TF_GetCode(status),
                 tensorflow::strings::StrCat(TF_Message(status), " [Op:",
                                             TFE_GetPythonString(op_name), "]")
                     .c_str());

    MaybeRaiseExceptionFromTFStatus(status, nullptr);
    return nullptr;
  }

  PyObject* flat_result = PyList_New(num_retvals);
  for (int i = 0; i < num_retvals; ++i) {
    PyList_SET_ITEM(flat_result, i, EagerTensorFromHandle(retvals[i]));
  }

  if (run_callbacks &&
      !RunCallbacks(run_gradient_callback, run_post_exec_callbacks, op_def,
                    args, *flattened_inputs, *flattened_attrs, flat_result,
                    op_name, name, record_gradient_callback, callbacks)) {
    return nullptr;
  }

  // Unflatten results.
  if (op_def->output_arg_size() == 0) {
    Py_RETURN_NONE;
  }

  if (op_def->output_arg_size() == 1) {
    if (!op_def->output_arg(0).number_attr().empty() ||
        !op_def->output_arg(0).type_list_attr().empty()) {
      return flat_result;
    } else {
      auto* result = PyList_GET_ITEM(flat_result, 0);
      Py_INCREF(result);
      Py_DECREF(flat_result);
      return result;
    }
  }

  // Correctly output the results that are made into a namedtuple.
  PyObject* result = PyList_New(op_def->output_arg_size());
  int flat_result_index = 0;
  for (int i = 0; i < op_def->output_arg_size(); i++) {
    if (!op_def->output_arg(i).number_attr().empty()) {
      int list_length = attr_list_sizes[op_def->output_arg(i).number_attr()];
      PyObject* inner_list = PyList_New(list_length);
      for (int j = 0; j < list_length; j++) {
        PyObject* obj = PyList_GET_ITEM(flat_result, flat_result_index++);
        Py_INCREF(obj);
        PyList_SET_ITEM(inner_list, j, obj);
      }
      PyList_SET_ITEM(result, i, inner_list);
    } else if (!op_def->output_arg(i).type_list_attr().empty()) {
      int list_length = attr_list_sizes[op_def->output_arg(i).type_list_attr()];
      PyObject* inner_list = PyList_New(list_length);
      for (int j = 0; j < list_length; j++) {
        PyObject* obj = PyList_GET_ITEM(flat_result, flat_result_index++);
        Py_INCREF(obj);
        PyList_SET_ITEM(inner_list, j, obj);
      }
      PyList_SET_ITEM(result, i, inner_list);
    } else {
      PyObject* obj = PyList_GET_ITEM(flat_result, flat_result_index++);
      Py_INCREF(obj);
      PyList_SET_ITEM(result, i, obj);
    }
  }
  Py_DECREF(flat_result);
  return result;
}
