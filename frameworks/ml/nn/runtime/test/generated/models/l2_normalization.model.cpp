// Generated file (from: l2_normalization.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type0(Type::TENSOR_FLOAT32, {1, 1, 1, 3});
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type0);
  // Phase 2, operations
  model->addOperation(ANEURALNETWORKS_L2_NORMALIZATION, {op1}, {op2});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {op1},
    {op2});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
