/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

package org.tensorflow.lite;

import java.util.Arrays;

/**
 * A typed multi-dimensional array used in Tensorflow Lite.
 *
 * <p>The native handle of a {@code Tensor} belongs to {@code NativeInterpreterWrapper}, thus not
 * needed to be closed here.
 */
final class Tensor {

  static Tensor fromHandle(long nativeHandle) {
    return new Tensor(nativeHandle);
  }

  /** Reads Tensor content into an array. */
  <T> T copyTo(T dst) {
    if (NativeInterpreterWrapper.dataTypeOf(dst) != dtype) {
      throw new IllegalArgumentException(
          String.format(
              "Cannot convert an TensorFlowLite tensor with type %s to a Java object of "
                  + "type %s (which is compatible with the TensorFlowLite type %s)",
              dtype, dst.getClass().getName(), NativeInterpreterWrapper.dataTypeOf(dst)));
    }
    int[] dstShape = NativeInterpreterWrapper.shapeOf(dst);
    if (!Arrays.equals(dstShape, shapeCopy)) {
      throw new IllegalArgumentException(
          String.format(
              "Shape of output target %s does not match with the shape of the Tensor %s.",
              Arrays.toString(dstShape), Arrays.toString(shapeCopy)));
    }
    readMultiDimensionalArray(nativeHandle, dst);
    return dst;
  }

  final long nativeHandle;
  final DataType dtype;
  final int[] shapeCopy;

  private Tensor(long nativeHandle) {
    this.nativeHandle = nativeHandle;
    this.dtype = DataType.fromNumber(dtype(nativeHandle));
    this.shapeCopy = shape(nativeHandle);
  }

  private static native int dtype(long handle);

  private static native int[] shape(long handle);

  private static native void readMultiDimensionalArray(long handle, Object value);

  static {
    TensorFlowLite.init();
  }
}
