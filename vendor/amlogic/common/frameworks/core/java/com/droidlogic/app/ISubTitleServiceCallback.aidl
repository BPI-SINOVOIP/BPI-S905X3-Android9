/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: ISubTitleServiceCallback
*/
package com.droidlogic.app;

interface ISubTitleServiceCallback
{
    void onSubTitleEvent(int event, int id);
    void onSubTitleAvailable(int available);
}
