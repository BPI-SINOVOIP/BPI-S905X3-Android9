/*
 * Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include <media/TypeConverter.h>

#include "policy.h"
#include <Volume.h>

namespace android {

struct DeviceCategoryTraits
{
    typedef device_category Type;
    typedef Vector<Type> Collection;
};
struct MixTypeTraits
{
    typedef int32_t Type;
    typedef Vector<Type> Collection;
};
struct RouteFlagTraits
{
    typedef uint32_t Type;
    typedef Vector<Type> Collection;
};
struct RuleTraits
{
    typedef uint32_t Type;
    typedef Vector<Type> Collection;
};

typedef TypeConverter<DeviceCategoryTraits> DeviceCategoryConverter;
typedef TypeConverter<MixTypeTraits> MixTypeConverter;
typedef TypeConverter<RouteFlagTraits> RouteFlagTypeConverter;
typedef TypeConverter<RuleTraits> RuleTypeConverter;

template <>
const DeviceCategoryConverter::Table DeviceCategoryConverter::mTable[];
template <>
const MixTypeConverter::Table MixTypeConverter::mTable[];
template <>
const RouteFlagTypeConverter::Table RouteFlagTypeConverter::mTable[];
template <>
const RuleTypeConverter::Table RuleTypeConverter::mTable[];

} // namespace android
