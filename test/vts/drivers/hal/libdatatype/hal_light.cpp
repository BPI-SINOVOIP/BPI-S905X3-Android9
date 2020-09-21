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

#include "hal_light.h"

#include <stdlib.h>

#include <iostream>

#include <hardware/lights.h>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

#include "vts_datatype.h"

using namespace std;

namespace android {
namespace vts {

light_state_t* GenerateLightState() {
  if (RandomBool()) {
    return NULL;
  } else {
    light_state_t* state = (light_state_t*)malloc(sizeof(light_state_t));

    state->color = RandomUint32();
    state->flashMode = RandomInt32();
    state->flashOnMS = RandomInt32();
    state->flashOffMS = RandomInt32();
    if (RandomBool()) {  // normal values
      if (RandomBool()) {
        state->brightnessMode = BRIGHTNESS_MODE_USER;
      } else {
        state->brightnessMode = BRIGHTNESS_MODE_SENSOR;
      }
    } else {  // abnormal values
      state->brightnessMode = RandomInt32();
    }

    return state;
  }
}

light_state_t* GenerateLightStateUsingMessage(
    const VariableSpecificationMessage& msg) {
  cout << __func__ << " entry" << endl;
  light_state_t* state = (light_state_t*)malloc(sizeof(light_state_t));

  // TODO: use a dict in the proto and handle when the key is missing (i.e.,
  // randomly generate that).
  state->color = msg.struct_value(0).scalar_value().uint32_t();
  cout << __func__ << " color " << state->color << endl;
  state->flashMode = msg.struct_value(1).scalar_value().int32_t();
  cout << __func__ << " flashMode " << state->flashMode << endl;
  state->flashOnMS = msg.struct_value(2).scalar_value().int32_t();
  cout << __func__ << " flashOnMS " << state->flashOnMS << endl;
  state->flashOffMS = msg.struct_value(3).scalar_value().int32_t();
  cout << __func__ << " flashOffMS " << state->flashOffMS << endl;
  state->brightnessMode = msg.struct_value(4).scalar_value().int32_t();
  cout << __func__ << " brightnessMode " << state->brightnessMode << endl;

  return state;
}

}  // namespace vts
}  // namespace android
