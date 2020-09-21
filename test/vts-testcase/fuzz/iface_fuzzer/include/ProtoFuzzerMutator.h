/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef __VTS_PROTO_FUZZER_MUTATOR_H_
#define __VTS_PROTO_FUZZER_MUTATOR_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "ProtoFuzzerRunner.h"
#include "ProtoFuzzerUtils.h"

namespace android {
namespace vts {
namespace fuzzer {

using BiasedRandomScalarGen = std::function<uint64_t(Random &rand)>;
using Odds = std::pair<uint64_t, uint64_t>;
using VarMutateFn = std::function<VarInstance(const VarInstance &)>;
using VarRandomGenFn = std::function<VarInstance(const VarSpec &)>;
using VarTransformFn = std::function<VariableSpecificationMessage(
    const VariableSpecificationMessage &)>;

// Encapsulates heuristic strategy for biased mutation/random generation.
struct ProtoFuzzerMutatorConfig {
  // Generates biased random scalars.
  BiasedRandomScalarGen scalar_bias_ = [](Random &rand) { return rand.Rand(); };
  // Used to decide if enum will be mutated/generated like a scalar.
  Odds enum_bias_ = {0, 1};
  // Odds that a function in an execution is mutated rather than regenerated.
  Odds func_mutated_ = {100, 1};
  // Default size used to randomly generate a vector.
  size_t default_vector_size_ = 64;
  // Default size used to randomly generate a string.
  size_t default_string_size_ = 16;
};

// Provides methods for mutation or random generation.
class ProtoFuzzerMutator {
 public:
  ProtoFuzzerMutator(Random &, std::unordered_map<std::string, TypeSpec>,
                     ProtoFuzzerMutatorConfig);
  // Generates a random ExecSpec.
  ExecSpec RandomGen(const IfaceDescTbl &, size_t);
  // Mutates in-place an ExecSpec.
  void Mutate(const IfaceDescTbl &, ExecSpec *);
  // Generates a random FuncSpec.
  FuncSpec RandomGen(const FuncSpec &);
  // Mutates a FuncSpec.
  FuncSpec Mutate(const FuncSpec &);
  // Generates a random VarInstance.
  VarInstance RandomGen(const VarSpec &);
  // Mutates a VarInstance.
  VarInstance Mutate(const VarInstance &);

 private:
  // Randomly selects an interface.
  const CompSpec *RandomSelectIface(const IfaceDescTbl &);

  // Used for mutation/random generation of VarInstance.
  VarInstance ArrayRandomGen(const VarSpec &);
  VarInstance ArrayMutate(const VarInstance &);
  VarInstance EnumRandomGen(const VarSpec &);
  VarInstance EnumMutate(const VarInstance &);
  VarInstance ScalarRandomGen(const VarSpec &);
  VarInstance ScalarMutate(const VarInstance &);
  VarInstance StringRandomGen(const VarSpec &);
  VarInstance StringMutate(const VarInstance &);
  VarInstance StructRandomGen(const VarSpec &);
  VarInstance StructMutate(const VarInstance &);
  VarInstance UnionRandomGen(const VarSpec &);
  VarInstance UnionMutate(const VarInstance &);
  VarInstance VectorRandomGen(const VarSpec &);
  VarInstance VectorMutate(const VarInstance &);

  // Used for mutation/random generation of ScalarData.
  ScalarData RandomGen(const ScalarData &, const std::string &);
  ScalarData Mutate(const ScalarData &, const string &);
  // Used for mutation/random generation of variables of fundamental data types
  // e.g. char, int, double.
  template <typename T>
  T RandomGen(T);
  template <typename T>
  T Mutate(T);
  bool RandomGen(bool);
  bool Mutate(bool);
  float Mutate(float);
  double Mutate(double);
  // Generates a random ASCII character.
  char RandomAsciiChar();

  // Looks up predefined type by name.
  const TypeSpec &FindPredefinedType(std::string);

  // 64-bit random number generator.
  Random &rand_;
  // Used to look up definition of a predefined type by its name.
  std::unordered_map<std::string, TypeSpec> predefined_types_;
  // Used to delegate mutation/random generation of VariableSpecifationMessage
  // of different VariableType to mutation/random delegation function for that
  // VariableType.
  std::unordered_map<VariableType, VarMutateFn> mutate_fns_;
  std::unordered_map<VariableType, VarRandomGenFn> random_gen_fns_;
  // Used for biased mutation/random generation of variables.
  ProtoFuzzerMutatorConfig mutator_config_;
};

}  // namespace fuzzer
}  // namespace vts
}  // namespace android

#endif  // __VTS_PROTO_FUZZER_MUTATOR__
