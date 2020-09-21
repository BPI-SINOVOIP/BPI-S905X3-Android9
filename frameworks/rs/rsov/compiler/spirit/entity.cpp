/*
 * Copyright 2017, The Android Open Source Project
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

#include "entity.h"

#include "instructions.h"
#include "visitor.h"
#include "word_stream.h"

namespace android {
namespace spirit {

void Entity::Serialize(OutputWordStream &OS) const {
  std::unique_ptr<IVisitor> v(CreateInstructionVisitor(
      [&OS](Instruction *inst) -> void { inst->Serialize(OS); }));
  v->visit(const_cast<Entity *>(this));
}

} // namespace spirit
} // namespace android
