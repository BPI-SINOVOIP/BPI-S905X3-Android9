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

#include "Debug.h"

#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "android-base/logging.h"
#include "android-base/stringprintf.h"

#include "ResourceTable.h"
#include "ResourceValues.h"
#include "ValueVisitor.h"
#include "text/Printer.h"
#include "util/Util.h"

using ::aapt::text::Printer;
using ::android::StringPiece;
using ::android::base::StringPrintf;

namespace aapt {

namespace {

class ValueHeadlinePrinter : public ConstValueVisitor {
 public:
  using ConstValueVisitor::Visit;

  explicit ValueHeadlinePrinter(const std::string& package, Printer* printer)
      : package_(package), printer_(printer) {
  }

  void Visit(const Attribute* attr) override {
    printer_->Print("(attr) type=");
    printer_->Print(attr->MaskString());
    if (!attr->symbols.empty()) {
      printer_->Print(StringPrintf(" size=%zd", attr->symbols.size()));
    }
  }

  void Visit(const Style* style) override {
    printer_->Print(StringPrintf("(style) size=%zd", style->entries.size()));
    if (style->parent) {
      printer_->Print(" parent=");

      const Reference& parent_ref = style->parent.value();
      if (parent_ref.name) {
        if (parent_ref.private_reference) {
          printer_->Print("*");
        }

        const ResourceName& parent_name = parent_ref.name.value();
        if (package_ != parent_name.package) {
          printer_->Print(parent_name.package);
          printer_->Print(":");
        }
        printer_->Print(to_string(parent_name.type));
        printer_->Print("/");
        printer_->Print(parent_name.entry);
        if (parent_ref.id) {
          printer_->Print(" (");
          printer_->Print(parent_ref.id.value().to_string());
          printer_->Print(")");
        }
      } else if (parent_ref.id) {
        printer_->Print(parent_ref.id.value().to_string());
      } else {
        printer_->Print("???");
      }
    }
  }

  void Visit(const Array* array) override {
    printer_->Print(StringPrintf("(array) size=%zd", array->elements.size()));
  }

  void Visit(const Plural* plural) override {
    size_t count = std::count_if(plural->values.begin(), plural->values.end(),
                                 [](const std::unique_ptr<Item>& v) { return v != nullptr; });
    printer_->Print(StringPrintf("(plurals) size=%zd", count));
  }

  void Visit(const Styleable* styleable) override {
    printer_->Println(StringPrintf("(styleable) size=%zd", styleable->entries.size()));
  }

  void VisitItem(const Item* item) override {
    // Pretty much guaranteed to be one line.
    if (const Reference* ref = ValueCast<Reference>(item)) {
      // Special case Reference so that we can print local resources without a package name.
      ref->PrettyPrint(package_, printer_);
    } else {
      item->PrettyPrint(printer_);
    }
  }

 private:
  std::string package_;
  Printer* printer_;
};

class ValueBodyPrinter : public ConstValueVisitor {
 public:
  using ConstValueVisitor::Visit;

  explicit ValueBodyPrinter(const std::string& package, Printer* printer)
      : package_(package), printer_(printer) {
  }

  void Visit(const Attribute* attr) override {
    constexpr uint32_t kMask = android::ResTable_map::TYPE_ENUM | android::ResTable_map::TYPE_FLAGS;
    if (attr->type_mask & kMask) {
      for (const auto& symbol : attr->symbols) {
        printer_->Print(symbol.symbol.name.value().entry);
        if (symbol.symbol.id) {
          printer_->Print("(");
          printer_->Print(symbol.symbol.id.value().to_string());
          printer_->Print(")");
        }
        printer_->Println(StringPrintf("=0x%08x", symbol.value));
      }
    }
  }

  void Visit(const Style* style) override {
    for (const auto& entry : style->entries) {
      if (entry.key.name) {
        const ResourceName& name = entry.key.name.value();
        if (!name.package.empty() && name.package != package_) {
          printer_->Print(name.package);
          printer_->Print(":");
        }
        printer_->Print(name.entry);

        if (entry.key.id) {
          printer_->Print("(");
          printer_->Print(entry.key.id.value().to_string());
          printer_->Print(")");
        }
      } else if (entry.key.id) {
        printer_->Print(entry.key.id.value().to_string());
      } else {
        printer_->Print("???");
      }

      printer_->Print("=");
      PrintItem(*entry.value);
      printer_->Println();
    }
  }

  void Visit(const Array* array) override {
    const size_t count = array->elements.size();
    printer_->Print("[");
    if (count > 0) {
      for (size_t i = 0u; i < count; i++) {
        if (i != 0u && i % 4u == 0u) {
          printer_->Println();
          printer_->Print(" ");
        }
        PrintItem(*array->elements[i]);
        if (i != count - 1) {
          printer_->Print(", ");
        }
      }
      printer_->Println("]");
    }
  }

  void Visit(const Plural* plural) override {
    constexpr std::array<const char*, Plural::Count> kPluralNames = {
        {"zero", "one", "two", "few", "many", "other"}};

    for (size_t i = 0; i < Plural::Count; i++) {
      if (plural->values[i] != nullptr) {
        printer_->Print(StringPrintf("%s=", kPluralNames[i]));
        PrintItem(*plural->values[i]);
        printer_->Println();
      }
    }
  }

  void Visit(const Styleable* styleable) override {
    for (const auto& attr : styleable->entries) {
      if (attr.name) {
        const ResourceName& name = attr.name.value();
        if (!name.package.empty() && name.package != package_) {
          printer_->Print(name.package);
          printer_->Print(":");
        }
        printer_->Print(name.entry);

        if (attr.id) {
          printer_->Print("(");
          printer_->Print(attr.id.value().to_string());
          printer_->Print(")");
        }
      }

      if (attr.id) {
        printer_->Print(attr.id.value().to_string());
      }
      printer_->Println();
    }
  }

  void VisitItem(const Item* item) override {
    // Intentionally left empty, we already printed the Items.
  }

 private:
  void PrintItem(const Item& item) {
    if (const Reference* ref = ValueCast<Reference>(&item)) {
      // Special case Reference so that we can print local resources without a package name.
      ref->PrettyPrint(package_, printer_);
    } else {
      item.PrettyPrint(printer_);
    }
  }

  std::string package_;
  Printer* printer_;
};

}  // namespace

void Debug::PrintTable(const ResourceTable& table, const DebugPrintTableOptions& options,
                       Printer* printer) {
  for (const auto& package : table.packages) {
    ValueHeadlinePrinter headline_printer(package->name, printer);
    ValueBodyPrinter body_printer(package->name, printer);

    printer->Print("Package name=");
    printer->Print(package->name);
    if (package->id) {
      printer->Print(StringPrintf(" id=%02x", package->id.value()));
    }
    printer->Println();

    printer->Indent();
    for (const auto& type : package->types) {
      printer->Print("type ");
      printer->Print(to_string(type->type));
      if (type->id) {
        printer->Print(StringPrintf(" id=%02x", type->id.value()));
      }
      printer->Println(StringPrintf(" entryCount=%zd", type->entries.size()));

      std::vector<const ResourceEntry*> sorted_entries;
      for (const auto& entry : type->entries) {
        auto iter = std::lower_bound(
            sorted_entries.begin(), sorted_entries.end(), entry.get(),
            [](const ResourceEntry* a, const ResourceEntry* b) -> bool {
              if (a->id && b->id) {
                return a->id.value() < b->id.value();
              } else if (a->id) {
                return true;
              } else {
                return false;
              }
            });
        sorted_entries.insert(iter, entry.get());
      }

      printer->Indent();
      for (const ResourceEntry* entry : sorted_entries) {
        const ResourceId id(package->id.value_or_default(0), type->id.value_or_default(0),
                            entry->id.value_or_default(0));

        printer->Print("resource ");
        printer->Print(id.to_string());
        printer->Print(" ");

        // Write the name without the package (this is obvious and too verbose).
        printer->Print(to_string(type->type));
        printer->Print("/");
        printer->Print(entry->name);

        switch (entry->visibility.level) {
          case Visibility::Level::kPublic:
            printer->Print(" PUBLIC");
            break;
          case Visibility::Level::kPrivate:
            printer->Print(" _PRIVATE_");
            break;
          case Visibility::Level::kUndefined:
            // Print nothing.
            break;
        }

        if (entry->overlayable) {
          printer->Print(" OVERLAYABLE");
        }

        printer->Println();

        if (options.show_values) {
          printer->Indent();
          for (const auto& value : entry->values) {
            printer->Print("(");
            printer->Print(value->config.to_string());
            printer->Print(") ");
            value->value->Accept(&headline_printer);
            if (options.show_sources && !value->value->GetSource().path.empty()) {
              printer->Print(" src=");
              printer->Print(value->value->GetSource().to_string());
            }
            printer->Println();
            printer->Indent();
            value->value->Accept(&body_printer);
            printer->Undent();
          }
          printer->Undent();
        }
      }
      printer->Undent();
    }
    printer->Undent();
  }
}

static size_t GetNodeIndex(const std::vector<ResourceName>& names, const ResourceName& name) {
  auto iter = std::lower_bound(names.begin(), names.end(), name);
  CHECK(iter != names.end());
  CHECK(*iter == name);
  return std::distance(names.begin(), iter);
}

void Debug::PrintStyleGraph(ResourceTable* table, const ResourceName& target_style) {
  std::map<ResourceName, std::set<ResourceName>> graph;

  std::queue<ResourceName> styles_to_visit;
  styles_to_visit.push(target_style);
  for (; !styles_to_visit.empty(); styles_to_visit.pop()) {
    const ResourceName& style_name = styles_to_visit.front();
    std::set<ResourceName>& parents = graph[style_name];
    if (!parents.empty()) {
      // We've already visited this style.
      continue;
    }

    Maybe<ResourceTable::SearchResult> result = table->FindResource(style_name);
    if (result) {
      ResourceEntry* entry = result.value().entry;
      for (const auto& value : entry->values) {
        if (Style* style = ValueCast<Style>(value->value.get())) {
          if (style->parent && style->parent.value().name) {
            parents.insert(style->parent.value().name.value());
            styles_to_visit.push(style->parent.value().name.value());
          }
        }
      }
    }
  }

  std::vector<ResourceName> names;
  for (const auto& entry : graph) {
    names.push_back(entry.first);
  }

  std::cout << "digraph styles {\n";
  for (const auto& name : names) {
    std::cout << "  node_" << GetNodeIndex(names, name) << " [label=\"" << name << "\"];\n";
  }

  for (const auto& entry : graph) {
    const ResourceName& style_name = entry.first;
    size_t style_node_index = GetNodeIndex(names, style_name);

    for (const auto& parent_name : entry.second) {
      std::cout << "  node_" << style_node_index << " -> "
                << "node_" << GetNodeIndex(names, parent_name) << ";\n";
    }
  }

  std::cout << "}" << std::endl;
}

void Debug::DumpHex(const void* data, size_t len) {
  const uint8_t* d = (const uint8_t*)data;
  for (size_t i = 0; i < len; i++) {
    std::cerr << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)d[i] << " ";
    if (i % 8 == 7) {
      std::cerr << "\n";
    }
  }

  if (len - 1 % 8 != 7) {
    std::cerr << std::endl;
  }
}

namespace {

class XmlPrinter : public xml::ConstVisitor {
 public:
  using xml::ConstVisitor::Visit;

  XmlPrinter(Printer* printer) : printer_(printer) {
  }

  void Visit(const xml::Element* el) override {
    for (const xml::NamespaceDecl& decl : el->namespace_decls) {
      printer_->Println(StringPrintf("N: %s=%s (line=%zu)", decl.prefix.c_str(), decl.uri.c_str(),
                                     decl.line_number));
      printer_->Indent();
    }

    printer_->Print("E: ");
    if (!el->namespace_uri.empty()) {
      printer_->Print(el->namespace_uri);
      printer_->Print(":");
    }
    printer_->Println(StringPrintf("%s (line=%zu)", el->name.c_str(), el->line_number));
    printer_->Indent();

    for (const xml::Attribute& attr : el->attributes) {
      printer_->Print("A: ");
      if (!attr.namespace_uri.empty()) {
        printer_->Print(attr.namespace_uri);
        printer_->Print(":");
      }
      printer_->Print(attr.name);

      if (attr.compiled_attribute) {
        printer_->Print("(");
        printer_->Print(
            attr.compiled_attribute.value().id.value_or_default(ResourceId(0)).to_string());
        printer_->Print(")");
      }
      printer_->Print("=");
      if (attr.compiled_value != nullptr) {
        attr.compiled_value->PrettyPrint(printer_);
      } else {
        printer_->Print("\"");
        printer_->Print(attr.value);
        printer_->Print("\"");
      }

      if (!attr.value.empty()) {
        printer_->Print(" (Raw: \"");
        printer_->Print(attr.value);
        printer_->Print("\")");
      }
      printer_->Println();
    }

    printer_->Indent();
    xml::ConstVisitor::Visit(el);
    printer_->Undent();
    printer_->Undent();

    for (size_t i = 0; i < el->namespace_decls.size(); i++) {
      printer_->Undent();
    }
  }

  void Visit(const xml::Text* text) override {
    printer_->Println(StringPrintf("T: '%s'", text->text.c_str()));
  }

 private:
  Printer* printer_;
};

}  // namespace

void Debug::DumpXml(const xml::XmlResource& doc, Printer* printer) {
  XmlPrinter xml_visitor(printer);
  doc.root->Accept(&xml_visitor);
}

}  // namespace aapt
