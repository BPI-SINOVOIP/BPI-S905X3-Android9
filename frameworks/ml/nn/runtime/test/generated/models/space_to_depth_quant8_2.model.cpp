// Generated file (from: space_to_depth_quant8_2.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type1(Type::INT32, {});
  OperandType type2(Type::TENSOR_QUANT8_ASYMM, {1, 2, 2, 4}, 0.5f, 0);
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {1, 4, 4, 1}, 0.5f, 0);
  // Phase 1, operands
  auto input = model->addOperand(&type0);
  auto radius = model->addOperand(&type1);
  auto output = model->addOperand(&type2);
  // Phase 2, operations
  static int32_t radius_init[] = {2};
  model->setOperandValue(radius, radius_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_SPACE_TO_DEPTH, {input, radius}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {input},
    {output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
