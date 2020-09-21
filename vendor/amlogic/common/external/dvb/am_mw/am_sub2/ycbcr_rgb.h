/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifndef YCBCR_RGB_H
#define YCBCR_RGB_H

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

#define CLIP(min, max, value)               (MAX(MIN(value,max),0))


/* the following calculation is suggested by Microsoft:
*
* Y = 0.257R¡ä + 0.504G¡ä + 0.098B¡ä + 16
* Cb = -0.148R¡ä - 0.291G¡ä + 0.439B¡ä + 128
* Cr = 0.439R¡ä - 0.368G¡ä - 0.071B¡ä + 128
*
* C = Y - 16
* D = U - 128
* E = V - 128
*
* R = clip(( 298 * C           + 409 * E + 128) >> 8)
* G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)
* B = clip(( 298 * C + 516 * D           + 128) >> 8)
*
* Using the previous coefficients and noting that clip() denotes clipping a value to the range of 0 to 255
* Note that R'G'B' here has full [0,255] range and YCbCroutput is normalized [16-235],[16-240]
* These formulas produce 8-bit results using coefficients that require no more than 8 bits of (unsigned) precision. Intermediate results require up to 16 bits of precision.
*/

#define RGB_TO_Y(r,g,b)						((unsigned char)((int)(66 * r + 129 * g + 25 * b + 128) >> 8) + 16)
#define RGB_TO_Cb(r,g,b)					((unsigned char)((int)(-38 * r - 74 * g + 112 * b + 128) >> 8) + 128)
#define RGB_TO_Cr(r,g,b)					((unsigned char)((int)(112 * r - 94 * g - 18 * b + 128) >> 8) + 128)

#define YCbCr_TO_R(y,cb,cr)					CLIP(0,255,((int)( 298 * (y-16)           + 409 * (cr-128) + 128) >> 8))
#define YCbCr_TO_G(y,cb,cr)					CLIP(0,255,((int)( 298 * (y-16) - 100 * (cb-128) - 208 * (cr-128) + 128) >> 8))
#define YCbCr_TO_B(y,cb,cr)					CLIP(0,255,((int)( 298 * (y-16) + 516 * (cb-128)           + 128) >> 8))


#endif

