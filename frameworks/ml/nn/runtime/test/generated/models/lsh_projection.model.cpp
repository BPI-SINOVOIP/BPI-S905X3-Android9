// Generated file (from: lsh_projection.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type3(Type::INT32, {});
  OperandType type2(Type::TENSOR_FLOAT32, {3});
  OperandType type0(Type::TENSOR_FLOAT32, {4, 2});
  OperandType type1(Type::TENSOR_INT32, {3, 2});
  OperandType type4(Type::TENSOR_INT32, {8});
  // Phase 1, operands
  auto hash = model->addOperand(&type0);
  auto lookup = model->addOperand(&type1);
  auto weight = model->addOperand(&type2);
  auto type_param = model->addOperand(&type3);
  auto output = model->addOperand(&type4);
  // Phase 2, operations
  static float hash_init[] = {0.123f, 0.456f, -0.321f, -0.654f, 1.234f, 5.678f, -4.321f, -8.765f};
  model->setOperandValue(hash, hash_init, sizeof(float) * 8);
  static int32_t type_param_init[] = {2};
  model->setOperandValue(type_param, type_param_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_LSH_PROJECTION, {hash, lookup, weight, type_param}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {lookup, weight},
    {output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
