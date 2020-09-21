// Generated file (from: space_to_batch_quant8_2.mod.py). Do not edit
// Begin of an example
{
//Input(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> FLOAT32 map
  {},
  // int -> INT32 map
  {},
  // int -> QUANT8_ASYMM map
  {{0, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}}
},
//Output(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> FLOAT32 map
  {},
  // int -> INT32 map
  {},
  // int -> QUANT8_ASYMM map
  {{0, {0, 0, 0, 5, 0, 0, 0, 6, 0, 1, 0, 7, 0, 2, 0, 8, 0, 3, 0, 9, 0, 4, 0, 10}}}
}
}, // End of an example
