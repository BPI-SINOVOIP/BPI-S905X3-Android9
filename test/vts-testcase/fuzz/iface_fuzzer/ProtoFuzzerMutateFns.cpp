/*
 * Copyright 2017 The Android Open Source Project
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

#include "ProtoFuzzerMutator.h"

using std::cerr;
using std::endl;

namespace android {
namespace vts {
namespace fuzzer {

// Creates an inital stub of mutation/random generation result.
static VarInstance VarInstanceStubFromSpec(const VarSpec &var_spec) {
  VarInstance result{};
  if (var_spec.has_type()) {
    result.set_type(var_spec.type());
  } else {
    cerr << "VarInstance with no type field: " << var_spec.DebugString();
    std::abort();
  }
  if (var_spec.has_name()) {
    result.set_name(var_spec.name());
  }
  if (var_spec.has_predefined_type()) {
    result.set_predefined_type(var_spec.predefined_type());
  }
  return result;
}

VarInstance ProtoFuzzerMutator::ArrayRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};
  size_t vector_size = var_spec.vector_size();
  result.set_vector_size(vector_size);
  for (size_t i = 0; i < vector_size; ++i) {
    *result.add_vector_value() = this->RandomGen(var_spec.vector_value(0));
  }
  return result;
}

VarInstance ProtoFuzzerMutator::ArrayMutate(const VarInstance &var_instance) {
  VarInstance result{var_instance};
  size_t vector_size = result.vector_size();
  size_t idx = rand_(vector_size);
  *result.mutable_vector_value(idx) = this->Mutate(result.vector_value(idx));
  return result;
}

VarInstance ProtoFuzzerMutator::EnumRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};
  const EnumData &blueprint =
      FindPredefinedType(result.predefined_type()).enum_value();

  size_t size = blueprint.enumerator_size();
  size_t idx = rand_(size);

  ScalarData scalar_value = blueprint.scalar_value(idx);
  string scalar_type = blueprint.scalar_type();

  // Mutate this enum like a scalar with probability
  // odds_for/(odds_for + odds_against).
  uint64_t odds_for = (this->mutator_config_).enum_bias_.first;
  uint64_t odds_against = (this->mutator_config_).enum_bias_.second;
  uint64_t rand_num = rand_(odds_for + odds_against);
  if (rand_num < odds_for) {
    scalar_value = Mutate(scalar_value, scalar_type);
  }

  *(result.mutable_scalar_value()) = scalar_value;
  result.set_scalar_type(scalar_type);
  return result;
}

VarInstance ProtoFuzzerMutator::EnumMutate(const VarInstance &var_instance) {
  // For TYPE_ENUM, VarInstance contains superset info of VarSpec.
  return RandomGen(var_instance);
}

VarInstance ProtoFuzzerMutator::ScalarRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};
  result.set_scalar_type(var_spec.scalar_type());
  (*result.mutable_scalar_value()) =
      RandomGen(result.scalar_value(), result.scalar_type());
  return result;
}

VarInstance ProtoFuzzerMutator::ScalarMutate(const VarInstance &var_instance) {
  VarInstance result{var_instance};
  (*result.mutable_scalar_value()) =
      Mutate(result.scalar_value(), result.scalar_type());
  return result;
}

VarInstance ProtoFuzzerMutator::StringRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};

  size_t str_size = mutator_config_.default_string_size_;
  string str(str_size, 0);
  auto rand_char = std::bind(&ProtoFuzzerMutator::RandomAsciiChar, this);
  std::generate_n(str.begin(), str_size, rand_char);

  StringDataValueMessage string_data;
  string_data.set_message(str.c_str());
  string_data.set_length(str_size);

  *result.mutable_string_value() = string_data;
  return result;
}

VarInstance ProtoFuzzerMutator::StringMutate(const VarInstance &var_instance) {
  VarInstance result{var_instance};
  string str = result.string_value().message();
  size_t str_size = result.string_value().length();

  // Three things can happen when mutating a string:
  // 1. A random char is inserted into a random position.
  // 2. A randomly selected char is removed from the string.
  // 3. A randomly selected char in the string is replaced by a random char.
  size_t dice_roll = str.empty() ? 0 : rand_(3);
  size_t idx = rand_(str_size);
  switch (dice_roll) {
    case 0:
      // Insert a random char.
      str.insert(str.begin() + idx, RandomAsciiChar());
      ++str_size;
      break;
    case 1:
      // Remove a randomly selected char.
      str.erase(str.begin() + idx);
      --str_size;
      break;
    case 2:
      // Replace a randomly selected char.
      str[idx] = RandomAsciiChar();
      break;
    default:
      // Do nothing.
      break;
  }

  result.mutable_string_value()->set_message(str);
  result.mutable_string_value()->set_length(str_size);
  return result;
}

VarInstance ProtoFuzzerMutator::StructRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};
  const TypeSpec &blueprint = FindPredefinedType(result.predefined_type());
  for (const VarSpec &struct_value : blueprint.struct_value()) {
    *result.add_struct_value() = this->RandomGen(struct_value);
  }
  return result;
}

VarInstance ProtoFuzzerMutator::StructMutate(const VarInstance &var_instance) {
  VarInstance result{var_instance};
  size_t size = result.struct_value_size();
  size_t idx = rand_(size);
  *result.mutable_struct_value(idx) = this->Mutate(result.struct_value(idx));
  return result;
}

VarInstance ProtoFuzzerMutator::UnionRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};
  const TypeSpec &blueprint = FindPredefinedType(result.predefined_type());
  size_t size = blueprint.union_value_size();
  for (size_t i = 0; i < size; ++i) {
    result.add_union_value();
  }
  size_t idx = rand_(size);
  *result.mutable_union_value(idx) =
      this->RandomGen(blueprint.union_value(idx));
  return result;
}

VarInstance ProtoFuzzerMutator::UnionMutate(const VarInstance &var_instance) {
  VarInstance result{var_instance};
  size_t size = result.union_value_size();
  for (size_t i = 0; i < size; ++i) {
    if (result.union_value(i).has_name()) {
      *result.mutable_union_value(i) = this->Mutate(result.union_value(i));
    }
  }
  return result;
}

VarInstance ProtoFuzzerMutator::VectorRandomGen(const VarSpec &var_spec) {
  VarInstance result{VarInstanceStubFromSpec(var_spec)};
  size_t size = mutator_config_.default_vector_size_;
  for (size_t i = 0; i < size; ++i) {
    *result.add_vector_value() = this->RandomGen(var_spec.vector_value(0));
  }
  return result;
}

VarInstance ProtoFuzzerMutator::VectorMutate(const VarInstance &var_instance) {
  VarInstance result{var_instance};
  size_t size = result.vector_size();
  size_t idx = rand_(size);
  *result.mutable_vector_value(idx) = this->Mutate(result.vector_value(idx));
  return result;
}

ScalarData ProtoFuzzerMutator::RandomGen(const ScalarData &scalar_value,
                                         const string &scalar_type) {
  ScalarData result{};

  if (scalar_type == "bool_t") {
    result.set_bool_t(RandomGen(static_cast<bool>(scalar_value.bool_t())));
  } else if (scalar_type == "int8_t") {
    result.set_int8_t(RandomGen(scalar_value.int8_t()));
  } else if (scalar_type == "uint8_t") {
    result.set_uint8_t(RandomGen(scalar_value.uint8_t()));
  } else if (scalar_type == "int16_t") {
    result.set_int16_t(RandomGen(scalar_value.int16_t()));
  } else if (scalar_type == "uint16_t") {
    result.set_uint16_t(RandomGen(scalar_value.uint16_t()));
  } else if (scalar_type == "int32_t") {
    result.set_int32_t(RandomGen(scalar_value.int32_t()));
  } else if (scalar_type == "uint32_t") {
    result.set_uint32_t(RandomGen(scalar_value.uint32_t()));
  } else if (scalar_type == "int64_t") {
    result.set_int64_t(RandomGen(scalar_value.int64_t()));
  } else if (scalar_type == "uint64_t") {
    result.set_uint64_t(RandomGen(scalar_value.uint64_t()));
  } else if (scalar_type == "float_t") {
    result.set_float_t(RandomGen(scalar_value.float_t()));
  } else if (scalar_type == "double_t") {
    result.set_double_t(RandomGen(scalar_value.double_t()));
  } else {
    cout << scalar_type << " not supported.\n";
  }

  return result;
}

ScalarData ProtoFuzzerMutator::Mutate(const ScalarData &scalar_value,
                                      const string &scalar_type) {
  ScalarData result{};

  if (scalar_type == "bool_t") {
    result.set_bool_t(Mutate(static_cast<bool>(scalar_value.bool_t())));
  } else if (scalar_type == "int8_t") {
    result.set_int8_t(Mutate(scalar_value.int8_t()));
  } else if (scalar_type == "uint8_t") {
    result.set_uint8_t(Mutate(scalar_value.uint8_t()));
  } else if (scalar_type == "int16_t") {
    result.set_int16_t(Mutate(scalar_value.int16_t()));
  } else if (scalar_type == "uint16_t") {
    result.set_uint16_t(Mutate(scalar_value.uint16_t()));
  } else if (scalar_type == "int32_t") {
    result.set_int32_t(Mutate(scalar_value.int32_t()));
  } else if (scalar_type == "uint32_t") {
    result.set_uint32_t(Mutate(scalar_value.uint32_t()));
  } else if (scalar_type == "int64_t") {
    result.set_int64_t(Mutate(scalar_value.int64_t()));
  } else if (scalar_type == "uint64_t") {
    result.set_uint64_t(Mutate(scalar_value.uint64_t()));
  } else if (scalar_type == "float_t") {
    result.set_float_t(Mutate(scalar_value.float_t()));
  } else if (scalar_type == "double_t") {
    result.set_double_t(Mutate(scalar_value.double_t()));
  } else {
    cout << scalar_type << " not supported.\n";
  }

  return result;
}

template <typename T>
T ProtoFuzzerMutator::RandomGen(T value) {
  // Generate a biased random scalar.
  uint64_t rand_int = mutator_config_.scalar_bias_(rand_);

  T result;
  memcpy(&result, &rand_int, sizeof(T));
  return result;
}

bool ProtoFuzzerMutator::RandomGen(bool value) {
  return static_cast<bool>(rand_(2));
}

template <typename T>
T ProtoFuzzerMutator::Mutate(T value) {
  size_t num_bits = 8 * sizeof(T);
  T mask = static_cast<T>(1) << rand_(num_bits);
  return value ^ mask;
}

bool ProtoFuzzerMutator::Mutate(bool value) { return RandomGen(value); }

float ProtoFuzzerMutator::Mutate(float value) {
  uint32_t copy;
  memcpy(&copy, &value, sizeof(float));
  uint32_t mask = static_cast<uint32_t>(1) << rand_(32);
  copy ^= mask;
  memcpy(&value, &copy, sizeof(float));
  return value;
}

double ProtoFuzzerMutator::Mutate(double value) {
  uint64_t copy;
  memcpy(&copy, &value, sizeof(double));
  uint64_t mask = static_cast<uint64_t>(1) << rand_(64);
  copy ^= mask;
  memcpy(&value, &copy, sizeof(double));
  return value;
}

char ProtoFuzzerMutator::RandomAsciiChar() {
  const char char_set[] =
      "0123456789"
      "`~!@#$%^&*()-_=+[{]};:',<.>/? "
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  size_t num_chars = sizeof(char_set) - 1;
  return char_set[rand_(num_chars)];
}

}  // namespace fuzzer
}  // namespace vts
}  // namespace android
