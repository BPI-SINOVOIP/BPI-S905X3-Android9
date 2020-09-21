// Generated file (from: dequantize.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type1(Type::TENSOR_FLOAT32, {1, 2, 2, 1});
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {1, 2, 2, 1}, 1.f, 0);
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type1);
  // Phase 2, operations
  model->addOperation(ANEURALNETWORKS_DEQUANTIZE, {op1}, {op2});
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
