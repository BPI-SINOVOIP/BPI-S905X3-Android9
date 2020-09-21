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


#include "ErrorUtils.h"

namespace android {

/**
   @brief Method to convert from POSIX to Android errors

   @param error Any of the standard POSIX error codes (defined in bionic/libc/kernel/common/asm-generic/errno.h)
   @return Any of the standard Android error code (defined in frameworks/base/include/utils/Errors.h)
 */
status_t ErrorUtils::posixToAndroidError(int error)
{
    switch(error)
        {
        case 0:
            return NO_ERROR;
        case EINVAL:
        case EFBIG:
        case EMSGSIZE:
        case E2BIG:
        case EFAULT:
        case EILSEQ:
            return BAD_VALUE;
        case ENOSYS:
            return INVALID_OPERATION;
        case EACCES:
        case EPERM:
            return PERMISSION_DENIED;
        case EADDRINUSE:
        case EAGAIN:
        case EALREADY:
        case EBUSY:
        case EEXIST:
        case EINPROGRESS:
            return ALREADY_EXISTS;
        case ENOMEM:
            return NO_MEMORY;
        default:
            return UNKNOWN_ERROR;
        };

    return NO_ERROR;
}

};



