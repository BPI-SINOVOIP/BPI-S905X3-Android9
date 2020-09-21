// Generated file (from: embedding_lookup.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type1(Type::TENSOR_FLOAT32, {3, 2, 4});
  OperandType type0(Type::TENSOR_INT32, {3});
  // Phase 1, operands
  auto index = model->addOperand(&type0);
  auto value = model->addOperand(&type1);
  auto output = model->addOperand(&type1);
  // Phase 2, operations
  model->addOperation(ANEURALNETWORKS_EMBEDDING_LOOKUP, {index, value}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {index, value},
    {output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
