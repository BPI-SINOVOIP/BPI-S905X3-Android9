/*
 * Copyright (C) 2017 The Android Open Source Project
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

/**
 * Tests for MIN/MAX vectorization.
 */
public class Main {

  /// CHECK-START: void Main.doitMin(float[], float[], float[]) loop_optimization (before)
  /// CHECK-DAG: <<Phi:i\d+>>  Phi                                 loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: <<Get1:f\d+>> ArrayGet                            loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: <<Get2:f\d+>> ArrayGet                            loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: <<Min:f\d+>>  InvokeStaticOrDirect [<<Get1>>,<<Get2>>] intrinsic:MathMinFloatFloat loop:<<Loop>> outer_loop:none
  /// CHECK-DAG:               ArraySet [{{l\d+}},<<Phi>>,<<Min>>] loop:<<Loop>>      outer_loop:none
  //
  // TODO x86: 0.0 vs -0.0?
  // TODO MIPS64: min(x, NaN)?
  //
  /// CHECK-START-ARM64: void Main.doitMin(float[], float[], float[]) loop_optimization (after)
  /// CHECK-DAG: <<Get1:d\d+>> VecLoad                              loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: <<Get2:d\d+>> VecLoad                              loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: <<Min:d\d+>>  VecMin [<<Get1>>,<<Get2>>]           loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG:               VecStore [{{l\d+}},{{i\d+}},<<Min>>] loop:<<Loop>>      outer_loop:none
  private static void doitMin(float[] x, float[] y, float[] z) {
    int min = Math.min(x.length, Math.min(y.length, z.length));
    for (int i = 0; i < min; i++) {
      x[i] = Math.min(y[i], z[i]);
    }
  }

  /// CHECK-START: void Main.doitMax(float[], float[], float[]) loop_optimization (before)
  /// CHECK-DAG: <<Phi:i\d+>>  Phi                                 loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: <<Get1:f\d+>> ArrayGet                            loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: <<Get2:f\d+>> ArrayGet                            loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: <<Max:f\d+>>  InvokeStaticOrDirect [<<Get1>>,<<Get2>>] intrinsic:MathMaxFloatFloat loop:<<Loop>> outer_loop:none
  /// CHECK-DAG:               ArraySet [{{l\d+}},<<Phi>>,<<Max>>] loop:<<Loop>>      outer_loop:none
  //
  // TODO x86: 0.0 vs -0.0?
  // TODO MIPS64: max(x, NaN)?
  //
  /// CHECK-START-ARM64: void Main.doitMax(float[], float[], float[]) loop_optimization (after)
  /// CHECK-DAG: <<Get1:d\d+>> VecLoad                              loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: <<Get2:d\d+>> VecLoad                              loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: <<Max:d\d+>>  VecMax [<<Get1>>,<<Get2>>]           loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG:               VecStore [{{l\d+}},{{i\d+}},<<Max>>] loop:<<Loop>>      outer_loop:none
  private static void doitMax(float[] x, float[] y, float[] z) {
    int min = Math.min(x.length, Math.min(y.length, z.length));
    for (int i = 0; i < min; i++) {
      x[i] = Math.max(y[i], z[i]);
    }
  }

  public static void main(String[] args) {
    float[] interesting = {
      -0.0f,
      +0.0f,
      -1.0f,
      +1.0f,
      -3.14f,
      +3.14f,
      -100.0f,
      +100.0f,
      -4444.44f,
      +4444.44f,
      Float.MIN_NORMAL,
      Float.MIN_VALUE,
      Float.MAX_VALUE,
      Float.NEGATIVE_INFINITY,
      Float.POSITIVE_INFINITY,
      Float.NaN
    };
    // Initialize cross-values for the interesting values.
    int total = interesting.length * interesting.length;
    float[] x = new float[total];
    float[] y = new float[total];
    float[] z = new float[total];
    int k = 0;
    for (int i = 0; i < interesting.length; i++) {
      for (int j = 0; j < interesting.length; j++) {
        x[k] = 0;
        y[k] = interesting[i];
        z[k] = interesting[j];
        k++;
      }
    }

    // And test.
    doitMin(x, y, z);
    for (int i = 0; i < total; i++) {
      float expected = Math.min(y[i], z[i]);
      expectEquals(expected, x[i]);
    }
    doitMax(x, y, z);
    for (int i = 0; i < total; i++) {
      float expected = Math.max(y[i], z[i]);
      expectEquals(expected, x[i]);
    }

    System.out.println("passed");
  }

  private static void expectEquals(float expected, float result) {
    // Tests the bits directly. This distinguishes correctly between +0.0
    // and -0.0 and returns a canonical representation for all NaN.
    int expected_bits = Float.floatToIntBits(expected);
    int result_bits = Float.floatToIntBits(result);
    if (expected_bits != result_bits) {
      throw new Error("Expected: " + expected +
          "(0x" + Integer.toHexString(expected_bits) + "), found: " + result +
          "(0x" + Integer.toHexString(result_bits) + ")");
    }
  }
}
