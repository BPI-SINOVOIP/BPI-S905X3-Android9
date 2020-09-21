// Generated file (from: avg_pool_float_4.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type1(Type::INT32, {});
  OperandType type2(Type::TENSOR_FLOAT32, {5, 11, 13, 3});
  OperandType type0(Type::TENSOR_FLOAT32, {5, 52, 60, 3});
  // Phase 1, operands
  auto i0 = model->addOperand(&type0);
  auto stride = model->addOperand(&type1);
  auto filter = model->addOperand(&type1);
  auto padding = model->addOperand(&type1);
  auto relu6_activation = model->addOperand(&type1);
  auto output = model->addOperand(&type2);
  // Phase 2, operations
  static int32_t stride_init[] = {5};
  model->setOperandValue(stride, stride_init, sizeof(int32_t) * 1);
  static int32_t filter_init[] = {100};
  model->setOperandValue(filter, filter_init, sizeof(int32_t) * 1);
  static int32_t padding_init[] = {50};
  model->setOperandValue(padding, padding_init, sizeof(int32_t) * 1);
  static int32_t relu6_activation_init[] = {3};
  model->setOperandValue(relu6_activation, relu6_activation_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_AVERAGE_POOL_2D, {i0, padding, padding, padding, padding, stride, stride, filter, filter, relu6_activation}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {i0},
    {output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
