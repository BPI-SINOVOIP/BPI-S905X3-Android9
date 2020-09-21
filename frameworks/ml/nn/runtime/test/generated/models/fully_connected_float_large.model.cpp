// Generated file (from: fully_connected_float_large.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type3(Type::INT32, {});
  OperandType type2(Type::TENSOR_FLOAT32, {1, 1});
  OperandType type0(Type::TENSOR_FLOAT32, {1, 5});
  OperandType type1(Type::TENSOR_FLOAT32, {1});
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type0);
  auto b0 = model->addOperand(&type1);
  auto op3 = model->addOperand(&type2);
  auto act = model->addOperand(&type3);
  // Phase 2, operations
  static float op2_init[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  model->setOperandValue(op2, op2_init, sizeof(float) * 5);
  static float b0_init[] = {900000.0f};
  model->setOperandValue(b0, b0_init, sizeof(float) * 1);
  static int32_t act_init[] = {0};
  model->setOperandValue(act, act_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {op1, op2, b0, act}, {op3});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {op1},
    {op3});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
