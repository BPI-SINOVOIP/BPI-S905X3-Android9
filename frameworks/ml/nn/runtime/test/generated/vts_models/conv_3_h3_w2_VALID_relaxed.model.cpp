// Generated code. Do not edit
// Create the model
Model createTestModel() {
    const std::vector<Operand> operands = {
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 0, .length = 4},
        },
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 4, .length = 4},
        },
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 8, .length = 4},
        },
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 12, .length = 4},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 8, 8, 3},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_INPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 6, 7, 3},
            .numberOfConsumers = 0,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_OUTPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {3, 3, 2, 3},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 16, .length = 216},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {3},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 232, .length = 12},
        }
    };

    const std::vector<Operation> operations = {
        {
            .type = OperationType::CONV_2D,
            .inputs = {4, 6, 7, 0, 1, 2, 3},
            .outputs = {5},
        }
    };

    const std::vector<uint32_t> inputIndexes = {4};
    const std::vector<uint32_t> outputIndexes = {5};
    std::vector<uint8_t> operandValues = {
      2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 188, 89, 119, 191, 42, 87, 20, 191, 153, 43, 47, 191, 185, 251, 60, 63, 177, 191, 60, 62, 8, 105, 199, 61, 147, 27, 53, 190, 202, 26, 117, 190, 233, 189, 116, 185, 52, 132, 99, 61, 230, 61, 110, 190, 181, 255, 161, 190, 76, 107, 83, 188, 114, 51, 164, 62, 150, 63, 167, 190, 193, 111, 107, 191, 142, 58, 94, 63, 131, 25, 83, 191, 193, 88, 239, 190, 124, 102, 228, 60, 94, 48, 16, 63, 177, 167, 197, 62, 228, 135, 138, 190, 144, 249, 112, 191, 108, 123, 71, 191, 72, 226, 133, 190, 142, 89, 70, 191, 65, 241, 75, 191, 159, 31, 102, 62, 180, 32, 212, 190, 242, 150, 47, 63, 90, 212, 167, 190, 150, 33, 70, 63, 149, 238, 54, 191, 234, 236, 120, 191, 163, 143, 142, 61, 143, 112, 82, 191, 105, 169, 76, 191, 112, 235, 190, 62, 77, 243, 106, 191, 47, 134, 82, 63, 207, 45, 20, 190, 85, 51, 43, 190, 108, 63, 137, 62, 72, 224, 51, 63, 229, 14, 211, 190, 108, 121, 65, 63, 78, 183, 56, 63, 227, 107, 223, 190, 89, 192, 140, 190, 255, 207, 137, 190, 109, 226, 36, 62, 38, 226, 81, 63, 131, 191, 159, 190, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    const std::vector<hidl_memory> pools = {};

    return {
        .operands = operands,
        .operations = operations,
        .inputIndexes = inputIndexes,
        .outputIndexes = outputIndexes,
        .operandValues = operandValues,
        .pools = pools,
        .relaxComputationFloat32toFloat16 = true,
    };
}


bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
