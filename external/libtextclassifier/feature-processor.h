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

// Feature processing for FFModel (feed-forward SmartSelection model).

#ifndef LIBTEXTCLASSIFIER_FEATURE_PROCESSOR_H_
#define LIBTEXTCLASSIFIER_FEATURE_PROCESSOR_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cached-features.h"
#include "model_generated.h"
#include "token-feature-extractor.h"
#include "tokenizer.h"
#include "types.h"
#include "util/base/integral_types.h"
#include "util/base/logging.h"
#include "util/utf8/unicodetext.h"
#include "util/utf8/unilib.h"

namespace libtextclassifier2 {

constexpr int kInvalidLabel = -1;

namespace internal {

TokenFeatureExtractorOptions BuildTokenFeatureExtractorOptions(
    const FeatureProcessorOptions* options);

// Splits tokens that contain the selection boundary inside them.
// E.g. "foo{bar}@google.com" -> "foo", "bar", "@google.com"
void SplitTokensOnSelectionBoundaries(CodepointSpan selection,
                                      std::vector<Token>* tokens);

// Returns the index of token that corresponds to the codepoint span.
int CenterTokenFromClick(CodepointSpan span, const std::vector<Token>& tokens);

// Returns the index of token that corresponds to the middle of the  codepoint
// span.
int CenterTokenFromMiddleOfSelection(
    CodepointSpan span, const std::vector<Token>& selectable_tokens);

// Strips the tokens from the tokens vector that are not used for feature
// extraction because they are out of scope, or pads them so that there is
// enough tokens in the required context_size for all inferences with a click
// in relative_click_span.
void StripOrPadTokens(TokenSpan relative_click_span, int context_size,
                      std::vector<Token>* tokens, int* click_pos);

// If unilib is not nullptr, just returns unilib. Otherwise, if unilib is
// nullptr, will create UniLib, assign ownership to owned_unilib, and return it.
const UniLib* MaybeCreateUnilib(const UniLib* unilib,
                                std::unique_ptr<UniLib>* owned_unilib);

}  // namespace internal

// Converts a codepoint span to a token span in the given list of tokens.
// If snap_boundaries_to_containing_tokens is set to true, it is enough for a
// token to overlap with the codepoint range to be considered part of it.
// Otherwise it must be fully included in the range.
TokenSpan CodepointSpanToTokenSpan(
    const std::vector<Token>& selectable_tokens, CodepointSpan codepoint_span,
    bool snap_boundaries_to_containing_tokens = false);

// Converts a token span to a codepoint span in the given list of tokens.
CodepointSpan TokenSpanToCodepointSpan(
    const std::vector<Token>& selectable_tokens, TokenSpan token_span);

// Takes care of preparing features for the span prediction model.
class FeatureProcessor {
 public:
  // A cache mapping codepoint spans to embedded tokens features. An instance
  // can be provided to multiple calls to ExtractFeatures() operating on the
  // same context (the same codepoint spans corresponding to the same tokens),
  // as an optimization. Note that the tokenizations do not have to be
  // identical.
  typedef std::map<CodepointSpan, std::vector<float>> EmbeddingCache;

  // If unilib is nullptr, will create and own an instance of a UniLib,
  // otherwise will use what's passed in.
  explicit FeatureProcessor(const FeatureProcessorOptions* options,
                            const UniLib* unilib = nullptr)
      : owned_unilib_(nullptr),
        unilib_(internal::MaybeCreateUnilib(unilib, &owned_unilib_)),
        feature_extractor_(internal::BuildTokenFeatureExtractorOptions(options),
                           *unilib_),
        options_(options),
        tokenizer_(
            options->tokenization_codepoint_config() != nullptr
                ? Tokenizer({options->tokenization_codepoint_config()->begin(),
                             options->tokenization_codepoint_config()->end()},
                            options->tokenize_on_script_change())
                : Tokenizer({}, /*split_on_script_change=*/false)) {
    MakeLabelMaps();
    if (options->supported_codepoint_ranges() != nullptr) {
      PrepareCodepointRanges({options->supported_codepoint_ranges()->begin(),
                              options->supported_codepoint_ranges()->end()},
                             &supported_codepoint_ranges_);
    }
    if (options->internal_tokenizer_codepoint_ranges() != nullptr) {
      PrepareCodepointRanges(
          {options->internal_tokenizer_codepoint_ranges()->begin(),
           options->internal_tokenizer_codepoint_ranges()->end()},
          &internal_tokenizer_codepoint_ranges_);
    }
    PrepareIgnoredSpanBoundaryCodepoints();
  }

  // Tokenizes the input string using the selected tokenization method.
  std::vector<Token> Tokenize(const std::string& text) const;

  // Same as above but takes UnicodeText.
  std::vector<Token> Tokenize(const UnicodeText& text_unicode) const;

  // Converts a label into a token span.
  bool LabelToTokenSpan(int label, TokenSpan* token_span) const;

  // Gets the total number of selection labels.
  int GetSelectionLabelCount() const { return label_to_selection_.size(); }

  // Gets the string value for given collection label.
  std::string LabelToCollection(int label) const;

  // Gets the total number of collections of the model.
  int NumCollections() const { return collection_to_label_.size(); }

  // Gets the name of the default collection.
  std::string GetDefaultCollection() const;

  const FeatureProcessorOptions* GetOptions() const { return options_; }

  // Retokenizes the context and input span, and finds the click position.
  // Depending on the options, might modify tokens (split them or remove them).
  void RetokenizeAndFindClick(const std::string& context,
                              CodepointSpan input_span,
                              bool only_use_line_with_click,
                              std::vector<Token>* tokens, int* click_pos) const;

  // Same as above but takes UnicodeText.
  void RetokenizeAndFindClick(const UnicodeText& context_unicode,
                              CodepointSpan input_span,
                              bool only_use_line_with_click,
                              std::vector<Token>* tokens, int* click_pos) const;

  // Returns true if the token span has enough supported codepoints (as defined
  // in the model config) or not and model should not run.
  bool HasEnoughSupportedCodepoints(const std::vector<Token>& tokens,
                                    TokenSpan token_span) const;

  // Extracts features as a CachedFeatures object that can be used for repeated
  // inference over token spans in the given context.
  bool ExtractFeatures(const std::vector<Token>& tokens, TokenSpan token_span,
                       CodepointSpan selection_span_for_feature,
                       const EmbeddingExecutor* embedding_executor,
                       EmbeddingCache* embedding_cache, int feature_vector_size,
                       std::unique_ptr<CachedFeatures>* cached_features) const;

  // Fills selection_label_spans with CodepointSpans that correspond to the
  // selection labels. The CodepointSpans are based on the codepoint ranges of
  // given tokens.
  bool SelectionLabelSpans(
      VectorSpan<Token> tokens,
      std::vector<CodepointSpan>* selection_label_spans) const;

  int DenseFeaturesCount() const {
    return feature_extractor_.DenseFeaturesCount();
  }

  int EmbeddingSize() const { return options_->embedding_size(); }

  // Splits context to several segments.
  std::vector<UnicodeTextRange> SplitContext(
      const UnicodeText& context_unicode) const;

  // Strips boundary codepoints from the span in context and returns the new
  // start and end indices. If the span comprises entirely of boundary
  // codepoints, the first index of span is returned for both indices.
  CodepointSpan StripBoundaryCodepoints(const std::string& context,
                                        CodepointSpan span) const;

  // Same as above but takes UnicodeText.
  CodepointSpan StripBoundaryCodepoints(const UnicodeText& context_unicode,
                                        CodepointSpan span) const;

 protected:
  // Represents a codepoint range [start, end).
  struct CodepointRange {
    int32 start;
    int32 end;

    CodepointRange(int32 arg_start, int32 arg_end)
        : start(arg_start), end(arg_end) {}
  };

  // Returns the class id corresponding to the given string collection
  // identifier. There is a catch-all class id that the function returns for
  // unknown collections.
  int CollectionToLabel(const std::string& collection) const;

  // Prepares mapping from collection names to labels.
  void MakeLabelMaps();

  // Gets the number of spannable tokens for the model.
  //
  // Spannable tokens are those tokens of context, which the model predicts
  // selection spans over (i.e., there is 1:1 correspondence between the output
  // classes of the model and each of the spannable tokens).
  int GetNumContextTokens() const { return options_->context_size() * 2 + 1; }

  // Converts a label into a span of codepoint indices corresponding to it
  // given output_tokens.
  bool LabelToSpan(int label, const VectorSpan<Token>& output_tokens,
                   CodepointSpan* span) const;

  // Converts a span to the corresponding label given output_tokens.
  bool SpanToLabel(const std::pair<CodepointIndex, CodepointIndex>& span,
                   const std::vector<Token>& output_tokens, int* label) const;

  // Converts a token span to the corresponding label.
  int TokenSpanToLabel(const std::pair<TokenIndex, TokenIndex>& span) const;

  void PrepareCodepointRanges(
      const std::vector<const FeatureProcessorOptions_::CodepointRange*>&
          codepoint_ranges,
      std::vector<CodepointRange>* prepared_codepoint_ranges);

  // Returns the ratio of supported codepoints to total number of codepoints in
  // the given token span.
  float SupportedCodepointsRatio(const TokenSpan& token_span,
                                 const std::vector<Token>& tokens) const;

  // Returns true if given codepoint is covered by the given sorted vector of
  // codepoint ranges.
  bool IsCodepointInRanges(
      int codepoint, const std::vector<CodepointRange>& codepoint_ranges) const;

  void PrepareIgnoredSpanBoundaryCodepoints();

  // Counts the number of span boundary codepoints. If count_from_beginning is
  // True, the counting will start at the span_start iterator (inclusive) and at
  // maximum end at span_end (exclusive). If count_from_beginning is True, the
  // counting will start from span_end (exclusive) and end at span_start
  // (inclusive).
  int CountIgnoredSpanBoundaryCodepoints(
      const UnicodeText::const_iterator& span_start,
      const UnicodeText::const_iterator& span_end,
      bool count_from_beginning) const;

  // Finds the center token index in tokens vector, using the method defined
  // in options_.
  int FindCenterToken(CodepointSpan span,
                      const std::vector<Token>& tokens) const;

  // Tokenizes the input text using ICU tokenizer.
  bool ICUTokenize(const UnicodeText& context_unicode,
                   std::vector<Token>* result) const;

  // Takes the result of ICU tokenization and retokenizes stretches of tokens
  // made of a specific subset of characters using the internal tokenizer.
  void InternalRetokenize(const UnicodeText& unicode_text,
                          std::vector<Token>* tokens) const;

  // Tokenizes a substring of the unicode string, appending the resulting tokens
  // to the output vector. The resulting tokens have bounds relative to the full
  // string. Does nothing if the start of the span is negative.
  void TokenizeSubstring(const UnicodeText& unicode_text, CodepointSpan span,
                         std::vector<Token>* result) const;

  // Removes all tokens from tokens that are not on a line (defined by calling
  // SplitContext on the context) to which span points.
  void StripTokensFromOtherLines(const std::string& context, CodepointSpan span,
                                 std::vector<Token>* tokens) const;

  // Same as above but takes UnicodeText.
  void StripTokensFromOtherLines(const UnicodeText& context_unicode,
                                 CodepointSpan span,
                                 std::vector<Token>* tokens) const;

  // Extracts the features of a token and appends them to the output vector.
  // Uses the embedding cache to to avoid re-extracting the re-embedding the
  // sparse features for the same token.
  bool AppendTokenFeaturesWithCache(const Token& token,
                                    CodepointSpan selection_span_for_feature,
                                    const EmbeddingExecutor* embedding_executor,
                                    EmbeddingCache* embedding_cache,
                                    std::vector<float>* output_features) const;

 private:
  std::unique_ptr<UniLib> owned_unilib_;
  const UniLib* unilib_;

 protected:
  const TokenFeatureExtractor feature_extractor_;

  // Codepoint ranges that define what codepoints are supported by the model.
  // NOTE: Must be sorted.
  std::vector<CodepointRange> supported_codepoint_ranges_;

  // Codepoint ranges that define which tokens (consisting of which codepoints)
  // should be re-tokenized with the internal tokenizer in the mixed
  // tokenization mode.
  // NOTE: Must be sorted.
  std::vector<CodepointRange> internal_tokenizer_codepoint_ranges_;

 private:
  // Set of codepoints that will be stripped from beginning and end of
  // predicted spans.
  std::set<int32> ignored_span_boundary_codepoints_;

  const FeatureProcessorOptions* const options_;

  // Mapping between token selection spans and labels ids.
  std::map<TokenSpan, int> selection_to_label_;
  std::vector<TokenSpan> label_to_selection_;

  // Mapping between collections and labels.
  std::map<std::string, int> collection_to_label_;

  Tokenizer tokenizer_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_FEATURE_PROCESSOR_H_
