/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "compile/XmlIdCollector.h"

#include <algorithm>
#include <vector>

#include "ResourceUtils.h"
#include "ResourceValues.h"
#include "xml/XmlDom.h"

namespace aapt {

namespace {

static bool cmp_name(const SourcedResourceName& a, const ResourceNameRef& b) {
  return a.name < b;
}

struct IdCollector : public xml::Visitor {
 public:
  using xml::Visitor::Visit;

  explicit IdCollector(std::vector<SourcedResourceName>* out_symbols)
      : out_symbols_(out_symbols) {}

  void Visit(xml::Element* element) override {
    for (xml::Attribute& attr : element->attributes) {
      ResourceNameRef name;
      bool create = false;
      if (ResourceUtils::ParseReference(attr.value, &name, &create, nullptr)) {
        if (create && name.type == ResourceType::kId) {
          auto iter = std::lower_bound(out_symbols_->begin(),
                                       out_symbols_->end(), name, cmp_name);
          if (iter == out_symbols_->end() || iter->name != name) {
            out_symbols_->insert(iter,
                                 SourcedResourceName{name.ToResourceName(),
                                                     element->line_number});
          }
        }
      }
    }

    xml::Visitor::Visit(element);
  }

 private:
  std::vector<SourcedResourceName>* out_symbols_;
};

}  // namespace

bool XmlIdCollector::Consume(IAaptContext* context, xml::XmlResource* xmlRes) {
  xmlRes->file.exported_symbols.clear();
  IdCollector collector(&xmlRes->file.exported_symbols);
  xmlRes->root->Accept(&collector);
  return true;
}

}  // namespace aapt
