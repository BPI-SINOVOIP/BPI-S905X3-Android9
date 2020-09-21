// Generated file (from: concat_quant8_2.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type2(Type::INT32, {});
  OperandType type1(Type::TENSOR_QUANT8_ASYMM, {40, 300}, 0.5f, 0);
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {52, 300}, 0.5f, 0);
  OperandType type3(Type::TENSOR_QUANT8_ASYMM, {92, 300}, 0.5f, 0);
  // Phase 1, operands
  auto input1 = model->addOperand(&type0);
  auto input2 = model->addOperand(&type1);
  auto axis0 = model->addOperand(&type2);
  auto output = model->addOperand(&type3);
  // Phase 2, operations
  static int32_t axis0_init[] = {0};
  model->setOperandValue(axis0, axis0_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_CONCATENATION, {input1, input2, axis0}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {input1, input2},
    {output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
