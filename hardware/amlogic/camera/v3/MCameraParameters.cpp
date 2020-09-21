/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef __MCAMERAPARAMETERS__
#define __MCAMERAPARAMETERS__

#define LOG_TAG "MCameraParameters"

#include <cutils/properties.h>
#include <math.h>
#include <utils/Errors.h>
#include <string.h>
#include <stdlib.h>
#include "MCameraParameters.h"

/*===========================================================================
 *  * FUNCTION   : MCameraParameters
 *  *
 *  * DESCRIPTION: constructor of MCameraParameters
 *  *
 *  * PARAMETERS : none
 *  *
 *  * RETURN     : None
 *  *==========================================================================*/
MCameraParameters::MCameraParameters()
            : CameraParameters(),mFd(-1),
{
}


/*===========================================================================
 *  * FUNCTION   : ~MCameraParameters
 *  *
 *  * DESCRIPTION: deconstructor of MCameraParameters
 *  *
 *  * PARAMETERS : String8
 *  *
 *  * RETURN     : None
 *  *==========================================================================*/
MCameraParameters::MCameraParameters(const String8 &params)
            : CameraParameters(params), mFd(-1)
{
}

/*===========================================================================
 *  * FUNCTION   : ~MCameraParameters
 *  *
 *  * DESCRIPTION: deconstructor of MCameraParameters
 *  *
 *  * PARAMETERS : String8, fd
 *  *
 *  * RETURN     : None
 *  *==========================================================================*/
MCameraParameters::MCameraParameters(const String8 &params, int fd)
            : CameraParameters(params), mFd(fd)
{
}




/*===========================================================================
 *  * FUNCTION   : ~MCameraParameters
 *  *
 *  * DESCRIPTION: deconstructor of MCameraParameters
 *  *
 *  * PARAMETERS : none
 *  *
 *  * RETURN     : None
 *  *==========================================================================*/
MCameraParameters::~MCameraParameters()
{

}
#endif
