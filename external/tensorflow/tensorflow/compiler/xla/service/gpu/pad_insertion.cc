/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/compiler/xla/service/gpu/pad_insertion.h"

#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/service/gpu/ir_emission_utils.h"
#include "tensorflow/compiler/xla/service/shape_inference.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/compiler/xla/window_util.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"

namespace xla {
namespace gpu {

namespace {
bool IsForwardConvolutionCanonical(const HloInstruction& conv) {
  CHECK_EQ(conv.custom_call_target(), kCudnnConvForwardCallTarget);
  return window_util::HasSymmetricPadding(conv.window()) &&
         !window_util::HasNegativePadding(conv.window()) &&
         !window_util::HasDilation(conv.window());
}

// If the (positive and negative) padding on the input operand of a convolution
// can't be folded into a cuDNN convolution libcall (e.g. uneven padding and
// dilation), returns kPad and/or kSlice instructions that explicitly apply the
// padding; otherwise returns the original input operand. When there is both
// positive padding (including dilation) and negative padding, we insert both
// kPad and kSlice.
HloInstruction* MaybePaddedAndSlicedInput(
    const Window& conv_window, const ConvolutionDimensionNumbers& conv_dnums,
    HloInstruction* input) {
  HloComputation* computation = input->parent();
  if (!window_util::HasSymmetricPadding(conv_window) ||
      window_util::HasBaseDilation(conv_window)) {
    // If padding is uneven or has dilation, we insert a kPad instruction that
    // applies positive padding and dilation.
    //
    // TODO(phawkins): If conv_window has asymmetric padding, perhaps instead of
    // moving all the padding into an explicit pad op, we should keep as much
    // padding inside of cudnn as possible, on the assumption that padding
    // within cudnn is basically free, whereas a kPad's cost increases as the
    // amount of padding increases.
    PaddingConfig padding_config =
        MakeNoPaddingConfig(input->shape().dimensions_size());
    for (size_t i = 0; i < conv_dnums.input_spatial_dimensions().size(); ++i) {
      int64 dim = conv_dnums.input_spatial_dimensions(i);
      padding_config.mutable_dimensions(dim)->set_edge_padding_low(
          std::max<int64>(0LL, conv_window.dimensions(i).padding_low()));
      padding_config.mutable_dimensions(dim)->set_edge_padding_high(
          std::max<int64>(0LL, conv_window.dimensions(i).padding_high()));
      padding_config.mutable_dimensions(dim)->set_interior_padding(
          conv_window.dimensions(i).base_dilation() - 1);
    }
    PrimitiveType element_type = input->shape().element_type();
    HloInstruction* padding =
        computation->AddInstruction(HloInstruction::CreateConstant(
            MakeUnique<Literal>(Literal::Zero(element_type))));
    input = computation->AddInstruction(HloInstruction::CreatePad(
        ShapeInference::InferPadShape(
            /*operand_shape=*/input->shape(),
            /*padding_value_shape=*/ShapeUtil::MakeShape(element_type, {}),
            padding_config)
            .ConsumeValueOrDie(),
        input, padding, padding_config));
  }

  if (window_util::HasNegativePadding(conv_window)) {
    // If the window has negative padding, insert a kSlice that explicitly
    // applies negative padding.
    //
    // For each dimension, initialize the start index to 0 and the limit index
    // to the size of that dimension.
    std::vector<int64> start_indices(input->shape().dimensions_size(), 0);
    std::vector<int64> limit_indices(input->shape().dimensions().begin(),
                                     input->shape().dimensions().end());
    std::vector<int64> strides(input->shape().dimensions_size(), 1);
    for (size_t i = 0; i < conv_dnums.input_spatial_dimensions().size(); ++i) {
      int64 dim = conv_dnums.input_spatial_dimensions(i);
      // If dimension "dim" has negative padding, increase the start index or
      // decrement the limit index by the amount of negative padding.
      start_indices[dim] +=
          std::max<int64>(0LL, -conv_window.dimensions(i).padding_low());
      limit_indices[dim] -=
          std::max<int64>(0LL, -conv_window.dimensions(i).padding_high());
    }

    input = computation->AddInstruction(HloInstruction::CreateSlice(
        ShapeInference::InferSliceShape(input->shape(), start_indices,
                                        limit_indices, strides)
            .ConsumeValueOrDie(),
        input, start_indices, limit_indices, strides));
  }

  return input;
}

// If the padding on the kernel operand of a convolution can't be folded into a
// cuDNN convolution libcall (e.g. dilation), returns a kPad instruction that
// explicitly applies the padding; otherwise returns the original kernel
// operand.
HloInstruction* MaybePaddedKernel(const Window& conv_window,
                                  const ConvolutionDimensionNumbers& conv_dnums,
                                  HloInstruction* kernel) {
  if (!window_util::HasWindowDilation(conv_window)) {
    return kernel;
  }

  // Compute the shape and padding config of the pad to be inserted.
  PaddingConfig padding_config;
  for (size_t i = 0; i < kernel->shape().dimensions_size(); ++i) {
    padding_config.add_dimensions();
  }
  for (size_t i = 0; i < conv_dnums.kernel_spatial_dimensions().size(); ++i) {
    int64 dim = conv_dnums.kernel_spatial_dimensions(i);
    padding_config.mutable_dimensions(dim)->set_interior_padding(
        conv_window.dimensions(i).window_dilation() - 1);
  }

  HloComputation* computation = kernel->parent();
  PrimitiveType element_type = kernel->shape().element_type();
  HloInstruction* padding =
      computation->AddInstruction(HloInstruction::CreateConstant(
          MakeUnique<Literal>(Literal::Zero(element_type))));
  return computation->AddInstruction(HloInstruction::CreatePad(
      ShapeInference::InferPadShape(
          /*operand_shape=*/kernel->shape(),
          /*padding_value_shape=*/ShapeUtil::MakeShape(element_type, {}),
          padding_config)
          .ConsumeValueOrDie(),
      kernel, padding, padding_config));
}
}  // namespace

bool PadInsertion::CanonicalizeForwardConvolution(HloInstruction* conv) {
  if (IsForwardConvolutionCanonical(*conv)) {
    return false;
  }

  // Insert slices and/or pads between the convolution and its input and/or
  // kernel operand.
  HloInstruction* new_input = MaybePaddedAndSlicedInput(
      conv->window(), conv->convolution_dimension_numbers(),
      conv->mutable_operand(0));
  HloInstruction* new_kernel =
      MaybePaddedKernel(conv->window(), conv->convolution_dimension_numbers(),
                        conv->mutable_operand(1));

  // Remove the padding from convolution's window field. These paddings are
  // made explicit with the inserted pads.
  Window new_conv_window = conv->window();
  for (size_t i = 0; i < new_conv_window.dimensions_size(); ++i) {
    WindowDimension* dim = new_conv_window.mutable_dimensions(i);

    // The size of the kernel may have changed so update the Window to match.
    dim->set_size(new_kernel->shape().dimensions(
        conv->convolution_dimension_numbers().kernel_spatial_dimensions(i)));
    dim->set_padding_low(0);
    dim->set_padding_high(0);
    dim->set_base_dilation(1);
    dim->set_window_dilation(1);
  }

  // The conv CustomCall returns a tuple (conv_result, scratch_buffer).  Extract
  // out the shape of conv_result.
  Shape old_conv_shape = conv->shape().tuple_shapes(0);

  VLOG(1) << "Canonicalizing forward conv";
  auto new_conv = CreateCudnnConvForward(old_conv_shape, new_input, new_kernel,
                                         new_conv_window,
                                         conv->convolution_dimension_numbers());
  VLOG(1) << "Replacing:\n  " << conv->ToString() << "\nwith:\n  "
          << new_conv->ToString();
  TF_CHECK_OK(conv->parent()->ReplaceInstruction(conv, new_conv));
  return true;
}

namespace {
void IncreasePaddingLowBy(int64 delta, WindowDimension* window_dim) {
  window_dim->set_padding_low(window_dim->padding_low() + delta);
}

void IncreasePaddingHighBy(int64 delta, WindowDimension* window_dim) {
  window_dim->set_padding_high(window_dim->padding_high() + delta);
}
}  // namespace

bool PadInsertion::CanonicalizeBackwardFilterConvolution(
    HloInstruction* backward_conv) {
  CHECK_EQ(backward_conv->custom_call_target(),
           kCudnnConvBackwardFilterCallTarget);
  if (window_util::HasSymmetricPadding(backward_conv->window())) {
    return false;
  }

  // A backward filter convolution with uneven padding can be canonicalized to
  // one with even padding by padding the activations (input) beforehand. For
  // example,
  //   BackwardFilterConv(ABCD, xyz, padding_low=1, padding_high=2)
  // is equivalent to
  //   ABCD0 = Pad(ABCD, padding_high=1)
  //   BackwardFilterConv(ABCD0, xyz, padding_low=pading_high=1)
  // We choose the lesser of padding_low and padding_high as the new padding.
  HloInstruction* input = backward_conv->mutable_operand(0);
  Window new_backward_conv_window = backward_conv->window();
  // input_padding_config is the config of the kPad to be inserted.
  PaddingConfig input_padding_config =
      MakeNoPaddingConfig(ShapeUtil::Rank(input->shape()));
  ConvolutionDimensionNumbers backward_conv_dnums =
      backward_conv->convolution_dimension_numbers();
  for (size_t i = 0; i < backward_conv->window().dimensions_size(); ++i) {
    int64 padding_low = backward_conv->window().dimensions(i).padding_low();
    int64 padding_high = backward_conv->window().dimensions(i).padding_high();
    if (padding_low < 0 || padding_high < 0) {
      // TODO(b/32744257): The following canonicalization wouldn't remove
      // negative padding in a backward convolution, and would therefore cause
      // cuDNN convolution (which doesn't support negative padding) to fail.
      return false;
    }
    // Compute the new, even padding for the backward conv operation.
    int64 new_conv_padding = std::min(padding_low, padding_high);
    int64 dim = backward_conv_dnums.input_spatial_dimensions(i);
    input_padding_config.mutable_dimensions(dim)->set_edge_padding_low(
        padding_low - new_conv_padding);
    input_padding_config.mutable_dimensions(dim)->set_edge_padding_high(
        padding_high - new_conv_padding);

    // Since we move some padding from the backward convolution to the kPad, we
    // need to accordingly reduce the padding amount of the backward convolution
    // and its inner forward convolution.
    auto* new_dim = new_backward_conv_window.mutable_dimensions(i);
    new_dim->set_padding_low(new_conv_padding);
    new_dim->set_padding_high(new_conv_padding);
  }

  // Create a new backward convolution replacing the old one.
  HloComputation* computation = backward_conv->parent();
  HloInstruction* output = backward_conv->mutable_operand(1);
  HloInstruction* padding =
      computation->AddInstruction(HloInstruction::CreateConstant(
          MakeUnique<Literal>(Literal::Zero(input->shape().element_type()))));
  HloInstruction* padded_input =
      computation->AddInstruction(HloInstruction::CreatePad(
          ShapeInference::InferPadShape(input->shape(), padding->shape(),
                                        input_padding_config)
              .ConsumeValueOrDie(),
          input, padding, input_padding_config));

  // The shape of the backward_conv CustomCall is a tuple (conv_result,
  // scratch_buffer).  Extract out the shape of conv_result.
  Shape backward_conv_shape = backward_conv->shape().tuple_shapes(0);
  HloInstruction* new_backward_conv = CreateCudnnConvBackwardFilter(
      backward_conv_shape, padded_input, output, new_backward_conv_window,
      backward_conv_dnums);

  VLOG(1) << "Canonicalizing backward filter conv";
  VLOG(1) << "Replacing:\n  " << backward_conv->ToString() << "\nwith:\n  "
          << new_backward_conv->ToString();

  TF_CHECK_OK(
      computation->ReplaceInstruction(backward_conv, new_backward_conv));
  return true;
}

bool PadInsertion::CanonicalizeBackwardInputConvolution(
    HloInstruction* backward_conv) {
  if (window_util::HasSymmetricPadding(backward_conv->window())) {
    return false;
  }

  Window new_backward_conv_window = backward_conv->window();
  ConvolutionDimensionNumbers backward_conv_dnums =
      backward_conv->convolution_dimension_numbers();

  // The backward_conv CustomCall returns a tuple (conv_result, scratch_memory).
  // Get the shape of conv_result.
  Shape backward_conv_shape = backward_conv->shape().tuple_shapes(0);

  Shape new_backward_conv_shape = backward_conv_shape;
  for (size_t i = 0; i < backward_conv->window().dimensions_size(); ++i) {
    int64 padding_low = backward_conv->window().dimensions(i).padding_low();
    int64 padding_high = backward_conv->window().dimensions(i).padding_high();
    if (padding_low < 0 || padding_high < 0) {
      // TODO(b/32744257): The following canonicalization wouldn't remove
      // negative padding in a backward convolution, and would therefore cause
      // cuDNN convolution (which doesn't support negative padding) to fail.
      return false;
    }
    // If the backward convolution has uneven padding on the activations, we
    // move some padding on the larger end to "internal" padding, so that the
    // backward convolution produces larger activations which get sliced later.
    //
    // For example, suppose we have a non-canonical HLO
    //   [A] = BackwardInputConvolve([a b], [x y z], padding=(low=2,high=1))
    // where the amount of padding low is larger, we can canonicalize it to
    //   [B A] = BackwardInputConvolve([a b], [x y z], padding=(low=1,high=1))
    //   [A] = Slice([B A])
    if (padding_low > padding_high) {
      IncreasePaddingLowBy(padding_high - padding_low,
                           new_backward_conv_window.mutable_dimensions(i));
    } else if (padding_low < padding_high) {
      IncreasePaddingHighBy(padding_low - padding_high,
                            new_backward_conv_window.mutable_dimensions(i));
    }
    // Decreasing the padding by X *increases* the size of our output by X.
    int64 dim = backward_conv_dnums.output_spatial_dimensions(i);
    new_backward_conv_shape.set_dimensions(
        dim, new_backward_conv_shape.dimensions(dim) +
                 std::abs(padding_low - padding_high));
  }

  // Create a new backward convolution replacing the old one.
  HloComputation* computation = backward_conv->parent();
  HloInstruction* output = backward_conv->mutable_operand(0);
  HloInstruction* filter = backward_conv->mutable_operand(1);

  HloInstruction* new_backward_conv_call = CreateCudnnConvBackwardInput(
      new_backward_conv_shape, output, filter, new_backward_conv_window,
      backward_conv_dnums);

  // The CustomCall created above returns a tuple (conv_result, scratch_memory).
  // Extract out the two elements.
  HloInstruction* new_backward_conv =
      computation->AddInstruction(HloInstruction::CreateGetTupleElement(
          new_backward_conv_shape, new_backward_conv_call, 0));
  HloInstruction* new_backward_conv_scratch =
      computation->AddInstruction(HloInstruction::CreateGetTupleElement(
          new_backward_conv_call->shape().tuple_shapes(1),
          new_backward_conv_call, 1));

  // Slice the new backward convolution.
  //
  // Initialize start_indices and limit_indices as no slicing.
  std::vector<int64> start_indices(new_backward_conv->shape().dimensions_size(),
                                   0LL);
  std::vector<int64> limit_indices(
      new_backward_conv->shape().dimensions().begin(),
      new_backward_conv->shape().dimensions().end());
  std::vector<int64> strides(new_backward_conv->shape().dimensions_size(), 1LL);
  for (size_t i = 0; i < backward_conv->window().dimensions_size(); ++i) {
    int64 padding_low = backward_conv->window().dimensions(i).padding_low();
    int64 padding_high = backward_conv->window().dimensions(i).padding_high();
    int64 dim = backward_conv_dnums.output_spatial_dimensions(i);
    if (padding_low > padding_high) {
      // If the amount of low padding (of the old backward convolution) is
      // larger, we internally pad the low end of the activations and slice
      // internal padding out here.
      start_indices[dim] += padding_low - padding_high;
    } else if (padding_low < padding_high) {
      // If the amount of high padding is larger, we slice out the internal
      // padding on the high end.
      limit_indices[dim] -= padding_high - padding_low;
    }
  }

  // Replace the old backward convolution with the slice.
  Shape slice_shape =
      ShapeInference::InferSliceShape(new_backward_conv->shape(), start_indices,
                                      limit_indices, strides)
          .ConsumeValueOrDie();
  CHECK(ShapeUtil::Compatible(slice_shape, backward_conv_shape))
      << ShapeUtil::HumanString(slice_shape) << " vs "
      << ShapeUtil::HumanString(backward_conv_shape);

  HloInstruction* slice = computation->AddInstruction(
      HloInstruction::CreateSlice(backward_conv_shape, new_backward_conv,
                                  start_indices, limit_indices, strides));
  HloInstruction* new_tuple = computation->AddInstruction(
      HloInstruction::CreateTuple({slice, new_backward_conv_scratch}));

  VLOG(1) << "Canonicalizing backward input conv";
  VLOG(1) << "Replacing:\n  " << backward_conv->ToString() << "\nwith:\n  "
          << new_tuple->ToString();

  TF_CHECK_OK(computation->ReplaceInstruction(backward_conv, new_tuple));
  return true;
}

StatusOr<bool> PadInsertion::Run(HloModule* module) {
  bool changed = false;
  for (HloInstruction* instruction :
       module->entry_computation()->MakeInstructionPostOrder()) {
    if (IsCustomCallToDnnConvolution(*instruction)) {
      const auto& target = instruction->custom_call_target();
      if (target == kCudnnConvForwardCallTarget) {
        changed |= CanonicalizeForwardConvolution(instruction);
      } else if (target == kCudnnConvBackwardFilterCallTarget) {
        changed |= CanonicalizeBackwardFilterConvolution(instruction);
      } else if (target == kCudnnConvBackwardInputCallTarget) {
        changed |= CanonicalizeBackwardInputConvolution(instruction);
      } else {
        LOG(FATAL) << "Unknown custom call target for cudnn conv: "
                   << instruction->ToString();
      }
    }
  }
  return changed;
}

}  // namespace gpu
}  // namespace xla
