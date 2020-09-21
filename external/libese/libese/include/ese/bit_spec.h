/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef ESE_BIT_SPEC_H__
#define ESE_BIT_SPEC_H__ 1

struct bit_spec {
  uint8_t value;
  uint8_t shift;
};

static inline uint8_t bs_get(const struct bit_spec b, uint8_t var) {
  return (var & (b.value << b.shift)) >> b.shift;
}

static inline uint8_t bs_mask(const struct bit_spec b, uint8_t value) {
  return (value & b.value) << b.shift;
}

static inline uint8_t bs_clear(const struct bit_spec b) {
 return (((uint8_t)~0) ^ bs_mask(b, b.value));
}

static inline uint8_t bs_set(uint8_t var, const struct bit_spec b, uint8_t value) {
  return ((var) & bs_clear(b)) | bs_mask(b, value);
}

static inline void bs_assign(uint8_t *dst, const struct bit_spec b, uint8_t value) {
  *dst =  bs_set(*dst, b, value);
}


#endif  /* ESE_BIT_SPEC_H__ */


