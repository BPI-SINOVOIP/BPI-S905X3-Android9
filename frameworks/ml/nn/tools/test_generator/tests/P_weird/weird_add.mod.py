# model
model = Model()
i1 = Input("op1", "TENSOR_FLOAT32", "{2}") # a vector of 2 float32s
i2 = Input("op2", "TENSOR_FLOAT32", "{2}") # another vector of 2 float32s
b0 = Int32Scalar("b0", 0) # an int32_t scalar bias
tmp = Internal("tmp", "TENSOR_FLOAT32", "{2}")
tmp2 = Internal("tmp2", "TENSOR_FLOAT32", "{2}")
o3 = Output("op3", "TENSOR_FLOAT32", "{2}")
i4 = Input("op4", "TENSOR_FLOAT32", "{2}") # another vector of 2 float32s
model = model.Operation("ADD", i1, i2, b0).To(tmp)
model = model.Operation("ADD", tmp, i2, b0).To(tmp2)
model = model.Operation("ADD", tmp2, i4, b0).To(o3)

# Example 1. Input in operand 0,
input0 = {i1: # input 0
          [1.0, 2.0],
          i2: # input 1
          [3.0, 4.0],
          i4: # input 4
          [5.0, 6.0]}

output0 = {o3: # output 0
           [9.0, 12.0]}

# Instantiate an example
Example((input0, output0))



