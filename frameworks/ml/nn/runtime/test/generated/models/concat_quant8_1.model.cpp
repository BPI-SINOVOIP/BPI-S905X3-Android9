// Generated file (from: concat_quant8_1.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type1(Type::INT32, {});
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {2, 3}, 0.5f, 0);
  OperandType type2(Type::TENSOR_QUANT8_ASYMM, {2, 6}, 0.5f, 0);
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type0);
  auto axis1 = model->addOperand(&type1);
  auto result = model->addOperand(&type2);
  // Phase 2, operations
  static int32_t axis1_init[] = {1};
  model->setOperandValue(axis1, axis1_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_CONCATENATION, {op1, op2, axis1}, {result});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {op1, op2},
    {result});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
