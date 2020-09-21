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

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>

#include "nugget/protobuf/options.pb.h"

using ::google::protobuf::FileDescriptor;
using ::google::protobuf::JoinStrings;
using ::google::protobuf::MethodDescriptor;
using ::google::protobuf::ServiceDescriptor;
using ::google::protobuf::Split;
using ::google::protobuf::SplitStringUsing;
using ::google::protobuf::StripSuffixString;
using ::google::protobuf::compiler::CodeGenerator;
using ::google::protobuf::compiler::OutputDirectory;
using ::google::protobuf::io::Printer;
using ::google::protobuf::io::ZeroCopyOutputStream;

using ::nugget::protobuf::app_id;
using ::nugget::protobuf::request_buffer_size;
using ::nugget::protobuf::response_buffer_size;

namespace {

std::string validateServiceOptions(const ServiceDescriptor& service) {
    if (!service.options().HasExtension(app_id)) {
        return "nugget.protobuf.app_id is not defined for service " + service.name();
    }
    if (!service.options().HasExtension(request_buffer_size)) {
        return "nugget.protobuf.request_buffer_size is not defined for service " + service.name();
    }
    if (!service.options().HasExtension(response_buffer_size)) {
        return "nugget.protobuf.response_buffer_size is not defined for service " + service.name();
    }
    return "";
}

template <typename Descriptor>
std::vector<std::string> Packages(const Descriptor& descriptor) {
    std::vector<std::string> namespaces;
    SplitStringUsing(descriptor.full_name(), ".", &namespaces);
    namespaces.pop_back(); // just take the package
    return namespaces;
}

template <typename Descriptor>
std::string FullyQualifiedIdentifier(const Descriptor& descriptor) {
    const auto namespaces = Packages(descriptor);
    if (namespaces.empty()) {
        return "::" + descriptor.name();
    } else {
        std::string namespace_path;
        JoinStrings(namespaces, "::", &namespace_path);
        return "::" + namespace_path + "::" + descriptor.name();
    }
}

template <typename Descriptor>
std::string FullyQualifiedHeader(const Descriptor& descriptor) {
    const auto packages = Packages(descriptor);
    const auto file = Split(descriptor.file()->name(), "/").back();
    const auto header = StripSuffixString(file, ".proto") + ".pb.h";
    if (packages.empty()) {
        return header;
    } else {
        std::string package_path;
        JoinStrings(packages, "/", &package_path);
        return package_path + "/" + header;
    }
}

template <typename Descriptor>
void OpenNamespaces(Printer& printer, const Descriptor& descriptor) {
    const auto namespaces = Packages(descriptor);
    for (const auto& ns : namespaces) {
        std::map<std::string, std::string> namespaceVars;
        namespaceVars["namespace"] = ns;
        printer.Print(namespaceVars, R"(
namespace $namespace$ {)");
    }
}

template <typename Descriptor>
void CloseNamespaces(Printer& printer, const Descriptor& descriptor) {
    const auto namespaces = Packages(descriptor);
    for (auto it = namespaces.crbegin(); it != namespaces.crend(); ++it) {
        std::map<std::string, std::string> namespaceVars;
        namespaceVars["namespace"] = *it;
        printer.Print(namespaceVars, R"(
} // namespace $namespace$)");
    }
}

void ForEachMethod(const ServiceDescriptor& service,
                   std::function<void(std::map<std::string, std::string>)> handler) {
    for (int i = 0; i < service.method_count(); ++i) {
        const MethodDescriptor& method = *service.method(i);
        std::map<std::string, std::string> vars;
        vars["method_id"] = std::to_string(i);
        vars["method_name"] = method.name();
        vars["method_input_type"] = FullyQualifiedIdentifier(*method.input_type());
        vars["method_output_type"] = FullyQualifiedIdentifier(*method.output_type());
        handler(vars);
    }
}

void GenerateMockClient(Printer& printer, const ServiceDescriptor& service) {
    std::map<std::string, std::string> vars;
    vars["include_guard"] = "PROTOC_GENERATED_MOCK_" + service.name() + "_CLIENT_H";
    vars["service_header"] = service.name() + ".client.h";
    vars["mock_class"] = "Mock" + service.name();
    vars["class"] = service.name();

    printer.Print(vars, R"(
#ifndef $include_guard$
#define $include_guard$

#include <gmock/gmock.h>

#include <$service_header$>)");

    OpenNamespaces(printer, service);

    printer.Print(vars, R"(
struct $mock_class$ : public I$class$ {)");

    ForEachMethod(service, [&](std::map<std::string, std::string> methodVars) {
        printer.Print(methodVars, R"(
    MOCK_METHOD2($method_name$, uint32_t(const $method_input_type$&, $method_output_type$*));)");
    });

    printer.Print(vars, R"(
};)");

    CloseNamespaces(printer, service);

    printer.Print(vars, R"(
#endif)");
}

void GenerateClientHeader(Printer& printer, const ServiceDescriptor& service) {
    std::map<std::string, std::string> vars;
    vars["include_guard"] = "PROTOC_GENERATED_" + service.name() + "_CLIENT_H";
    vars["protobuf_header"] = FullyQualifiedHeader(service);
    vars["class"] = service.name();
    vars["iface_class"] = "I" + service.name();
    vars["app_id"] = "APP_ID_" + service.options().GetExtension(app_id);

    printer.Print(vars, R"(
#ifndef $include_guard$
#define $include_guard$

#include <application.h>
#include <nos/AppClient.h>
#include <nos/NuggetClientInterface.h>

#include "$protobuf_header$")");

    OpenNamespaces(printer, service);

    // Pure virtual interface to make testing easier
    printer.Print(vars, R"(
class $iface_class$ {
public:
    virtual ~$iface_class$() = default;)");

    ForEachMethod(service, [&](std::map<std::string, std::string> methodVars) {
        printer.Print(methodVars, R"(
    virtual uint32_t $method_name$(const $method_input_type$&, $method_output_type$*) = 0;)");
    });

    printer.Print(vars, R"(
};)");

    // Implementation of the interface for Nugget
    printer.Print(vars, R"(
class $class$ : public $iface_class$ {
    ::nos::AppClient _app;
public:
    $class$(::nos::NuggetClientInterface& client) : _app{client, $app_id$} {}
    ~$class$() override = default;)");

    ForEachMethod(service, [&](std::map<std::string, std::string> methodVars) {
        printer.Print(methodVars, R"(
    uint32_t $method_name$(const $method_input_type$&, $method_output_type$*) override;)");
    });

    printer.Print(vars, R"(
};)");

    CloseNamespaces(printer, service);

    printer.Print(vars, R"(
#endif)");
}

void GenerateClientSource(Printer& printer, const ServiceDescriptor& service) {
    std::map<std::string, std::string> vars;
    vars["generated_header"] = service.name() + ".client.h";
    vars["class"] = service.name();

    const uint32_t max_request_size = service.options().GetExtension(request_buffer_size);
    const uint32_t max_response_size = service.options().GetExtension(response_buffer_size);
    vars["max_request_size"] = std::to_string(max_request_size);
    vars["max_response_size"] = std::to_string(max_response_size);

    printer.Print(vars, R"(
#include <$generated_header$>

#include <application.h>)");

    OpenNamespaces(printer, service);

    // Methods
    ForEachMethod(service, [&](std::map<std::string, std::string>  methodVars) {
        methodVars.insert(vars.begin(), vars.end());
        printer.Print(methodVars, R"(
uint32_t $class$::$method_name$(const $method_input_type$& request, $method_output_type$* response) {
    const size_t request_size = request.ByteSize();
    if (request_size > $max_request_size$) {
        return APP_ERROR_TOO_MUCH;
    }
    std::vector<uint8_t> buffer(request_size);
    if (!request.SerializeToArray(buffer.data(), buffer.size())) {
        return APP_ERROR_RPC;
    }
    std::vector<uint8_t> responseBuffer;
    if (response != nullptr) {
      responseBuffer.resize($max_response_size$);
    }
    const uint32_t appStatus = _app.Call($method_id$, buffer,
                                         (response != nullptr) ? &responseBuffer : nullptr);
    if (appStatus == APP_SUCCESS && response != nullptr) {
        if (!response->ParseFromArray(responseBuffer.data(), responseBuffer.size())) {
            return APP_ERROR_RPC;
        }
    }
    return appStatus;
})");
    });

    CloseNamespaces(printer, service);
}

// Generator for C++ Nugget service client
class CppNuggetServiceClientGenerator : public CodeGenerator {
public:
    CppNuggetServiceClientGenerator() = default;
    ~CppNuggetServiceClientGenerator() override = default;

    bool Generate(const FileDescriptor* file,
                  const std::string& parameter,
                  OutputDirectory* output_directory,
                  std::string* error) const override {
        for (int i = 0; i < file->service_count(); ++i) {
            const auto& service = *file->service(i);

            *error = validateServiceOptions(service);
            if (!error->empty()) {
                return false;
            }

            if (parameter == "mock") {
                std::unique_ptr<ZeroCopyOutputStream> output{
                        output_directory->Open("Mock" + service.name() + ".client.h")};
                Printer printer(output.get(), '$');
                GenerateMockClient(printer, service);
            } else if (parameter == "header") {
                std::unique_ptr<ZeroCopyOutputStream> output{
                        output_directory->Open(service.name() + ".client.h")};
                Printer printer(output.get(), '$');
                GenerateClientHeader(printer, service);
            } else if (parameter == "source") {
                std::unique_ptr<ZeroCopyOutputStream> output{
                        output_directory->Open(service.name() + ".client.cpp")};
                Printer printer(output.get(), '$');
                GenerateClientSource(printer, service);
            } else {
                *error = "Illegal parameter: must be mock|header|source";
                return false;
            }
        }

        return true;
    }

private:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CppNuggetServiceClientGenerator);
};

} // namespace

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    CppNuggetServiceClientGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
