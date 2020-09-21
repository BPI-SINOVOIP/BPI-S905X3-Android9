#include "test/vts/specification/lib/ndk/bionic/1.0/libcV1.vts.h"
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <linux/socket.h>
#include "vts_datatype.h"
#include "vts_measurement.h"
#include <android-base/logging.h>


namespace android {
namespace vts {
bool FuzzerExtended_libc::Fuzz(
    FunctionSpecificationMessage* func_msg,
    void** result, const string& callback_socket_name) {
    const char* func_name = func_msg->name().c_str();
    LOG(INFO) << "Function: " << func_name;
    if (!strcmp(func_name, "socket")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        int32_t arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_int32_t()) ? func_msg->arg(1).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg1 = " << arg1;
        int32_t arg2 = (func_msg->arg(2).type() == TYPE_SCALAR && func_msg->arg(2).scalar_value().has_int32_t()) ? func_msg->arg(2).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_socket)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_socket) target_loader_.GetLoaderFunction("socket"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "accept")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        struct sockaddr* arg1 = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    LOG(INFO) << "arg1 = " << arg1;
        socklen_t* arg2 = (socklen_t*) malloc(sizeof(socklen_t));
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_accept)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_accept) target_loader_.GetLoaderFunction("accept"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "bind")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        struct sockaddr* arg1 = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    LOG(INFO) << "arg1 = " << arg1;
        socklen_t* arg2 = (socklen_t*) malloc(sizeof(socklen_t));
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_bind)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_bind) target_loader_.GetLoaderFunction("bind"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "connect")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        struct sockaddr* arg1 = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    LOG(INFO) << "arg1 = " << arg1;
        socklen_t* arg2 = (socklen_t*) malloc(sizeof(socklen_t));
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_connect)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_connect) target_loader_.GetLoaderFunction("connect"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "listen")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        int32_t arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_int32_t()) ? func_msg->arg(1).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg1 = " << arg1;
        typedef void* (*func_type_listen)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_listen) target_loader_.GetLoaderFunction("listen"))(
          arg0,
          arg1)));
        return true;
      }
    if (!strcmp(func_name, "recv")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        void* arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_void_pointer()) ? reinterpret_cast<void*>(func_msg->arg(1).scalar_value().void_pointer()) : RandomVoidPointer();
    LOG(INFO) << "arg1 = " << arg1;
        uint32_t arg2 = (func_msg->arg(2).type() == TYPE_SCALAR && func_msg->arg(2).scalar_value().has_uint32_t()) ? func_msg->arg(2).scalar_value().uint32_t() : RandomUint32();
    LOG(INFO) << "arg2 = " << arg2;
        int32_t arg3 = (func_msg->arg(3).type() == TYPE_SCALAR && func_msg->arg(3).scalar_value().has_int32_t()) ? func_msg->arg(3).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg3 = " << arg3;
        typedef void* (*func_type_recv)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_recv) target_loader_.GetLoaderFunction("recv"))(
          arg0,
          arg1,
          arg2,
          arg3)));
        return true;
      }
    if (!strcmp(func_name, "send")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        void* arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_void_pointer()) ? reinterpret_cast<void*>(func_msg->arg(1).scalar_value().void_pointer()) : RandomVoidPointer();
    LOG(INFO) << "arg1 = " << arg1;
        uint32_t arg2 = (func_msg->arg(2).type() == TYPE_SCALAR && func_msg->arg(2).scalar_value().has_uint32_t()) ? func_msg->arg(2).scalar_value().uint32_t() : RandomUint32();
    LOG(INFO) << "arg2 = " << arg2;
        int32_t arg3 = (func_msg->arg(3).type() == TYPE_SCALAR && func_msg->arg(3).scalar_value().has_int32_t()) ? func_msg->arg(3).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg3 = " << arg3;
        typedef void* (*func_type_send)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_send) target_loader_.GetLoaderFunction("send"))(
          arg0,
          arg1,
          arg2,
          arg3)));
        return true;
      }
    if (!strcmp(func_name, "fopen")) {
        char arg0[func_msg->arg(0).string_value().length() + 1];
        if (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).string_value().has_message()) {
          strcpy(arg0, func_msg->arg(0).string_value().message().c_str());
        } else {
       strcpy(arg0, RandomCharPointer());
        }
    ;
    LOG(INFO) << "arg0 = " << arg0;
        char arg1[func_msg->arg(1).string_value().length() + 1];
        if (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).string_value().has_message()) {
          strcpy(arg1, func_msg->arg(1).string_value().message().c_str());
        } else {
       strcpy(arg1, RandomCharPointer());
        }
    ;
    LOG(INFO) << "arg1 = " << arg1;
        typedef void* (*func_type_fopen)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_fopen) target_loader_.GetLoaderFunction("fopen"))(
          arg0,
          arg1)));
        return true;
      }
    if (!strcmp(func_name, "read")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        void* arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_void_pointer()) ? reinterpret_cast<void*>(func_msg->arg(1).scalar_value().void_pointer()) : RandomVoidPointer();
    LOG(INFO) << "arg1 = " << arg1;
        uint32_t arg2 = (func_msg->arg(2).type() == TYPE_SCALAR && func_msg->arg(2).scalar_value().has_uint32_t()) ? func_msg->arg(2).scalar_value().uint32_t() : RandomUint32();
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_read)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_read) target_loader_.GetLoaderFunction("read"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "write")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        void* arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_void_pointer()) ? reinterpret_cast<void*>(func_msg->arg(1).scalar_value().void_pointer()) : RandomVoidPointer();
    LOG(INFO) << "arg1 = " << arg1;
        int32_t arg2 = (func_msg->arg(2).type() == TYPE_SCALAR && func_msg->arg(2).scalar_value().has_int32_t()) ? func_msg->arg(2).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_write)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_write) target_loader_.GetLoaderFunction("write"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "lseek")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        int32_t arg1 = (func_msg->arg(1).type() == TYPE_SCALAR && func_msg->arg(1).scalar_value().has_int32_t()) ? func_msg->arg(1).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg1 = " << arg1;
        int32_t arg2 = (func_msg->arg(2).type() == TYPE_SCALAR && func_msg->arg(2).scalar_value().has_int32_t()) ? func_msg->arg(2).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg2 = " << arg2;
        typedef void* (*func_type_lseek)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_lseek) target_loader_.GetLoaderFunction("lseek"))(
          arg0,
          arg1,
          arg2)));
        return true;
      }
    if (!strcmp(func_name, "close")) {
        int32_t arg0 = (func_msg->arg(0).type() == TYPE_SCALAR && func_msg->arg(0).scalar_value().has_int32_t()) ? func_msg->arg(0).scalar_value().int32_t() : RandomInt32();
    LOG(INFO) << "arg0 = " << arg0;
        typedef void* (*func_type_close)(...);
    *result = const_cast<void*>(reinterpret_cast<const void*>(    ((func_type_close) target_loader_.GetLoaderFunction("close"))(
          arg0)));
        return true;
      }
    return false;
}
bool FuzzerExtended_libc::GetAttribute(
    FunctionSpecificationMessage* func_msg,
    void** result) {
    const char* func_name = func_msg->name().c_str();
    LOG(INFO) << " '" << func_name << "'";
    LOG(ERROR) << "attribute not supported for shared lib yet.";
    return false;
}
bool FuzzerExtended_libc::CallFunction(const FunctionSpecificationMessage&, const string&, FunctionSpecificationMessage* ) {
    /* No implementation yet. */
    return true;
}
bool FuzzerExtended_libc::VerifyResults(const FunctionSpecificationMessage&, const FunctionSpecificationMessage&) {
    /* No implementation yet. */
    return true;
}
extern "C" {
android::vts::DriverBase* vts_func_11_1002_V1_0_() {
    return (android::vts::DriverBase*) new android::vts::FuzzerExtended_libc();
}

}
}  // namespace vts
}  // namespace android
