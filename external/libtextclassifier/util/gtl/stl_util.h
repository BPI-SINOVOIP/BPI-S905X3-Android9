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

#ifndef LIBTEXTCLASSIFIER_UTIL_GTL_STL_UTIL_H_
#define LIBTEXTCLASSIFIER_UTIL_GTL_STL_UTIL_H_

namespace libtextclassifier2 {

// Deletes all the elements in an STL container and clears the container. This
// function is suitable for use with a vector, set, hash_set, or any other STL
// container which defines sensible begin(), end(), and clear() methods.
// If container is NULL, this function is a no-op.
template <typename T>
void STLDeleteElements(T *container) {
  if (!container) return;
  auto it = container->begin();
  while (it != container->end()) {
    auto temp = it;
    ++it;
    delete *temp;
  }
  container->clear();
}

// Given an STL container consisting of (key, value) pairs, STLDeleteValues
// deletes all the "value" components and clears the container. Does nothing in
// the case it's given a nullptr.
template <typename T>
void STLDeleteValues(T *container) {
  if (!container) return;
  auto it = container->begin();
  while (it != container->end()) {
    auto temp = it;
    ++it;
    delete temp->second;
  }
  container->clear();
}

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_GTL_STL_UTIL_H_
