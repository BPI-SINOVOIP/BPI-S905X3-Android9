// Generated file (from: conv_quant8_weights_as_inputs.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type3(Type::INT32, {});
  OperandType type2(Type::TENSOR_INT32, {1}, 0.25f, 0);
  OperandType type1(Type::TENSOR_QUANT8_ASYMM, {1, 2, 2, 1}, 0.5f, 0);
  OperandType type4(Type::TENSOR_QUANT8_ASYMM, {1, 2, 2, 1}, 1.f, 0);
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {1, 3, 3, 1}, 0.5f, 0);
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type1);
  auto op3 = model->addOperand(&type2);
  auto pad0 = model->addOperand(&type3);
  auto act = model->addOperand(&type3);
  auto stride = model->addOperand(&type3);
  auto op4 = model->addOperand(&type4);
  // Phase 2, operations
  static int32_t pad0_init[] = {0};
  model->setOperandValue(pad0, pad0_init, sizeof(int32_t) * 1);
  static int32_t act_init[] = {0};
  model->setOperandValue(act, act_init, sizeof(int32_t) * 1);
  static int32_t stride_init[] = {1};
  model->setOperandValue(stride, stride_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_CONV_2D, {op1, op2, op3, pad0, pad0, pad0, pad0, stride, stride, act}, {op4});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {op1, op2, op3},
    {op4});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
