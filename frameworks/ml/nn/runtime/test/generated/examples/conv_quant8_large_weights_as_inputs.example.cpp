// Generated file (from: conv_quant8_large_weights_as_inputs.mod.py). Do not edit
// Begin of an example
{
//Input(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> FLOAT32 map
  {},
  // int -> INT32 map
  {{2, {0, 0, 0}}},
  // int -> QUANT8_ASYMM map
  {{0, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}}, {1, {1, 4, 7, 2, 5, 8, 3, 6, 9}}}
},
//Output(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> FLOAT32 map
  {},
  // int -> INT32 map
  {},
  // int -> QUANT8_ASYMM map
  {{0, {8, 9, 11, 17, 21, 24, 26, 32, 38, 35, 43, 51, 44, 54, 65, 53, 66, 78}}}
}
}, // End of an example
