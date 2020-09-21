/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef _EDGEPOINTERUTIL_H_
#define _EDGEPOINTERUTIL_H_

typedef short EdgePointer;

inline EdgePointer sym(EdgePointer a)
{
  return a ^ 2;
}

inline EdgePointer rot(EdgePointer a)
{
  return (((a) + 1) & 3) | ((a) & ~3);
}

inline EdgePointer rotinv(EdgePointer a)
{
  return (((a) + 3) & 3) | ((a) & ~3);
}

#endif //_EDGEPOINTERUTIL_H_
