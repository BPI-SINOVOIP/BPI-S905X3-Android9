/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#include <stdio.h>
#include <stdlib.h>

#include "ubootenv/Ubootenv.h"

int main(int argc, char *argv[])
{
    Ubootenv *ubootenv = new Ubootenv();
    if (argc == 1) {
        ubootenv->printValues();
    } else {
        //printf("key:%s\n", argv[1]);
        const char* p_value = ubootenv->getValue(argv[1]);
        if (p_value) {
            printf("%s\n", p_value);
        }
    }
    return 0;
}
