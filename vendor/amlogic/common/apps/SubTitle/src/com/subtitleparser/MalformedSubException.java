/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;

/**
 * SUB format exception.
 *
 * @author
 */
public class MalformedSubException extends Exception {

        public MalformedSubException (String msg) {
            super (msg);
        }
}
