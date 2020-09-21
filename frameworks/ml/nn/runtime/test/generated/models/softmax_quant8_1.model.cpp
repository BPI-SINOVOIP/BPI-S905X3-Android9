// Generated file (from: softmax_quant8_1.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type1(Type::FLOAT32, {});
  OperandType type2(Type::TENSOR_QUANT8_ASYMM, {1, 4}, 0.00390625f, 0);
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {1, 4}, 0.5f, 0);
  // Phase 1, operands
  auto input = model->addOperand(&type0);
  auto beta = model->addOperand(&type1);
  auto output = model->addOperand(&type2);
  // Phase 2, operations
  static float beta_init[] = {1e-05f};
  model->setOperandValue(beta, beta_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_SOFTMAX, {input, beta}, {output});
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
