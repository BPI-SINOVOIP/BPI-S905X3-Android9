// Generated file (from: reshape.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type0(Type::TENSOR_FLOAT32, {1, 1, 3, 3});
  OperandType type2(Type::TENSOR_FLOAT32, {9});
  OperandType type1(Type::TENSOR_INT32, {1});
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type1);
  auto op3 = model->addOperand(&type2);
  // Phase 2, operations
  static int32_t op2_init[] = {-1};
  model->setOperandValue(op2, op2_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_RESHAPE, {op1, op2}, {op3});
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
