/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Contains classes that can execute different models/parts of a model.

#ifndef LIBTEXTCLASSIFIER_MODEL_EXECUTOR_H_
#define LIBTEXTCLASSIFIER_MODEL_EXECUTOR_H_

#include <memory>

#include "tensor-view.h"
#include "types.h"
#include "util/base/logging.h"
#include "tensorflow/contrib/lite/interpreter.h"
#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"

namespace libtextclassifier2 {

namespace internal {
bool FromModelSpec(const tflite::Model* model_spec,
                   std::unique_ptr<const tflite::FlatBufferModel>* model);
}  // namespace internal

// A helper function that given indices of feature and logits tensor, feature
// values computes the logits using given interpreter.
TensorView<float> ComputeLogitsHelper(const int input_index_features,
                                      const int output_index_logits,
                                      const TensorView<float>& features,
                                      tflite::Interpreter* interpreter);

// Executor for the text selection prediction and classification models.
class ModelExecutor {
 public:
  static std::unique_ptr<const ModelExecutor> Instance(
      const flatbuffers::Vector<uint8_t>* model_spec_buffer) {
    const tflite::Model* model =
        flatbuffers::GetRoot<tflite::Model>(model_spec_buffer->data());
    flatbuffers::Verifier verifier(model_spec_buffer->data(),
                                   model_spec_buffer->Length());
    if (!model->Verify(verifier)) {
      return nullptr;
    }
    return Instance(model);
  }

  static std::unique_ptr<const ModelExecutor> Instance(
      const tflite::Model* model_spec) {
    std::unique_ptr<const tflite::FlatBufferModel> model;
    if (!internal::FromModelSpec(model_spec, &model)) {
      return nullptr;
    }
    return std::unique_ptr<ModelExecutor>(new ModelExecutor(std::move(model)));
  }

  // Creates an Interpreter for the model that serves as a scratch-pad for the
  // inference. The Interpreter is NOT thread-safe.
  std::unique_ptr<tflite::Interpreter> CreateInterpreter() const;

  TensorView<float> ComputeLogits(const TensorView<float>& features,
                                  tflite::Interpreter* interpreter) const {
    return ComputeLogitsHelper(kInputIndexFeatures, kOutputIndexLogits,
                               features, interpreter);
  }

 protected:
  explicit ModelExecutor(std::unique_ptr<const tflite::FlatBufferModel> model)
      : model_(std::move(model)) {}

  static const int kInputIndexFeatures = 0;
  static const int kOutputIndexLogits = 0;

  std::unique_ptr<const tflite::FlatBufferModel> model_;
  tflite::ops::builtin::BuiltinOpResolver builtins_;
};

// Executor for embedding sparse features into a dense vector.
class EmbeddingExecutor {
 public:
  virtual ~EmbeddingExecutor() {}

  // Embeds the sparse_features into a dense embedding and adds (+) it
  // element-wise to the dest vector.
  virtual bool AddEmbedding(const TensorView<int>& sparse_features, float* dest,
                            int dest_size) const = 0;

  // Returns true when the model is ready to be used, false otherwise.
  virtual bool IsReady() const { return true; }
};

class TFLiteEmbeddingExecutor : public EmbeddingExecutor {
 public:
  static std::unique_ptr<TFLiteEmbeddingExecutor> Instance(
      const flatbuffers::Vector<uint8_t>* model_spec_buffer, int embedding_size,
      int quantization_bits);

  bool AddEmbedding(const TensorView<int>& sparse_features, float* dest,
                    int dest_size) const override;

 protected:
  explicit TFLiteEmbeddingExecutor(
      std::unique_ptr<const tflite::FlatBufferModel> model,
      int quantization_bits, int num_buckets, int bytes_per_embedding,
      int output_embedding_size, const TfLiteTensor* scales,
      const TfLiteTensor* embeddings,
      std::unique_ptr<tflite::Interpreter> interpreter);

  std::unique_ptr<const tflite::FlatBufferModel> model_;

  int quantization_bits_;
  int num_buckets_ = -1;
  int bytes_per_embedding_ = -1;
  int output_embedding_size_ = -1;
  const TfLiteTensor* scales_ = nullptr;
  const TfLiteTensor* embeddings_ = nullptr;

  // NOTE: This interpreter is used in a read-only way (as a storage for the
  // model params), thus is still thread-safe.
  std::unique_ptr<tflite::Interpreter> interpreter_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_MODEL_EXECUTOR_H_
