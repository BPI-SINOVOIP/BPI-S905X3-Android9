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

#include "tests/test_data.h"

namespace android {
namespace aidl {
namespace test_data {
namespace string_constants {

const char kCanonicalName[] = "android.os.IStringConstants";
const char kInterfaceDefinition[] = R"(
package android.os;

interface IStringConstants {
  const String EXAMPLE_CONSTANT = "foo";
}
)";

const char kJavaOutputPath[] = "some/path/to/output.java";
const char kExpectedJavaOutput[] =
R"(/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Original file: android/os/IStringConstants.aidl
 */
package android.os;
public interface IStringConstants extends android.os.IInterface
{
/** Local-side IPC implementation stub class. */
public static abstract class Stub extends android.os.Binder implements android.os.IStringConstants
{
private static final java.lang.String DESCRIPTOR = "android.os.IStringConstants";
/** Construct the stub at attach it to the interface. */
public Stub()
{
this.attachInterface(this, DESCRIPTOR);
}
/**
 * Cast an IBinder object into an android.os.IStringConstants interface,
 * generating a proxy if needed.
 */
public static android.os.IStringConstants asInterface(android.os.IBinder obj)
{
if ((obj==null)) {
return null;
}
android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
if (((iin!=null)&&(iin instanceof android.os.IStringConstants))) {
return ((android.os.IStringConstants)iin);
}
return new android.os.IStringConstants.Stub.Proxy(obj);
}
@Override public android.os.IBinder asBinder()
{
return this;
}
@Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
{
java.lang.String descriptor = DESCRIPTOR;
switch (code)
{
case INTERFACE_TRANSACTION:
{
reply.writeString(descriptor);
return true;
}
default:
{
return super.onTransact(code, data, reply, flags);
}
}
}
private static class Proxy implements android.os.IStringConstants
{
private android.os.IBinder mRemote;
Proxy(android.os.IBinder remote)
{
mRemote = remote;
}
@Override public android.os.IBinder asBinder()
{
return mRemote;
}
public java.lang.String getInterfaceDescriptor()
{
return DESCRIPTOR;
}
}
}
public static final String EXAMPLE_CONSTANT = "foo";
}
)";

const char kCppOutputPath[] = "some/path/to/output.cpp";
const char kGenHeaderDir[] = "output";
const char kGenInterfaceHeaderPath[] = "output/android/os/IStringConstants.h";
const char kExpectedIHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_I_STRING_CONSTANTS_H_
#define AIDL_GENERATED_ANDROID_OS_I_STRING_CONSTANTS_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android {

namespace os {

class IStringConstants : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(StringConstants)
static const ::android::String16& EXAMPLE_CONSTANT();
};  // class IStringConstants

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_I_STRING_CONSTANTS_H_
)";

const char kExpectedCppOutput[] =
R"(#include <android/os/IStringConstants.h>
#include <android/os/BpStringConstants.h>

namespace android {

namespace os {

IMPLEMENT_META_INTERFACE(StringConstants, "android.os.IStringConstants")

const ::android::String16& IStringConstants::EXAMPLE_CONSTANT() {
static const ::android::String16 value("foo");
return value;
}

}  // namespace os

}  // namespace android
#include <android/os/BpStringConstants.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

BpStringConstants::BpStringConstants(const ::android::sp<::android::IBinder>& _aidl_impl)
    : BpInterface<IStringConstants>(_aidl_impl){
}

}  // namespace os

}  // namespace android
#include <android/os/BnStringConstants.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

::android::status_t BnStringConstants::onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) {
::android::status_t _aidl_ret_status = ::android::OK;
switch (_aidl_code) {
default:
{
_aidl_ret_status = ::android::BBinder::onTransact(_aidl_code, _aidl_data, _aidl_reply, _aidl_flags);
}
break;
}
if (_aidl_ret_status == ::android::UNEXPECTED_NULL) {
_aidl_ret_status = ::android::binder::Status::fromExceptionCode(::android::binder::Status::EX_NULL_POINTER).writeToParcel(_aidl_reply);
}
return _aidl_ret_status;
}

}  // namespace os

}  // namespace android
)";

}  // namespace string_constants
}  // namespace test_data
}  // namespace aidl
}  // namespace android
