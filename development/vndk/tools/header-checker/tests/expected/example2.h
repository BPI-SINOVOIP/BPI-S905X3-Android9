record_types {
  type_info {
    name: "HiddenBase"
    size: 8
    alignment: 4
    referenced_type: "type-1"
    source_file: "/development/vndk/tools/header-checker/tests/input/example3.h"
    linker_set_key: "HiddenBase"
    self_type: "type-1"
  }
  fields {
    referenced_type: "type-2"
    field_offset: 0
    field_name: "hide"
    access: private_access
  }
  fields {
    referenced_type: "type-3"
    field_offset: 32
    field_name: "seek"
    access: private_access
  }
  access: public_access
  record_kind: class_kind
  tag_info {
    unique_id: "_ZTS10HiddenBase"
  }
}
record_types {
  type_info {
    name: "test2::HelloAgain"
    size: 40
    alignment: 8
    referenced_type: "type-4"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test2::HelloAgain"
    self_type: "type-4"
  }
  fields {
    referenced_type: "type-5"
    field_offset: 64
    field_name: "foo_again"
    access: public_access
  }
  fields {
    referenced_type: "type-2"
    field_offset: 256
    field_name: "bar_again"
    access: public_access
  }
  vtable_layout {
    vtable_components {
      kind: OffsetToTop
      mangled_component_name: ""
      component_value: 0
    }
    vtable_components {
      kind: RTTI
      mangled_component_name: "_ZTIN5test210HelloAgainE"
      component_value: 0
    }
    vtable_components {
      kind: FunctionPointer
      mangled_component_name: "_ZN5test210HelloAgain5againEv"
      component_value: 0
    }
    vtable_components {
      kind: CompleteDtorPointer
      mangled_component_name: "_ZN5test210HelloAgainD1Ev"
      component_value: 0
    }
    vtable_components {
      kind: DeletingDtorPointer
      mangled_component_name: "_ZN5test210HelloAgainD0Ev"
      component_value: 0
    }
  }
  access: public_access
  record_kind: struct_kind
  tag_info {
    unique_id: "_ZTSN5test210HelloAgainE"
  }
}
record_types {
  type_info {
    name: "test3::ByeAgain<double>"
    size: 16
    alignment: 8
    referenced_type: "type-13"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test3::ByeAgain<double>"
    self_type: "type-13"
  }
  fields {
    referenced_type: "type-14"
    field_offset: 0
    field_name: "foo_again"
    access: public_access
  }
  fields {
    referenced_type: "type-2"
    field_offset: 64
    field_name: "bar_again"
    access: public_access
  }
  template_info {
    elements {
      referenced_type: "type-14"
    }
  }
  access: public_access
  record_kind: struct_kind
  tag_info {
    unique_id: "_ZTSN5test38ByeAgainIdEE"
  }
}
record_types {
  type_info {
    name: "test3::ByeAgain<float>"
    size: 8
    alignment: 4
    referenced_type: "type-15"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test3::ByeAgain<float>"
    self_type: "type-15"
  }
  fields {
    referenced_type: "type-3"
    field_offset: 0
    field_name: "foo_again"
    access: public_access
  }
  fields {
    referenced_type: "type-3"
    field_offset: 32
    field_name: "bar_Again"
    access: public_access
  }
  template_info {
    elements {
      referenced_type: "type-3"
    }
  }
  access: public_access
  record_kind: struct_kind
  tag_info {
    unique_id: "_ZTSN5test38ByeAgainIfEE"
  }
}
record_types {
  type_info {
    name: "test3::Outer"
    size: 4
    alignment: 4
    referenced_type: "type-17"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test3::Outer"
    self_type: "type-17"
  }
  fields {
    referenced_type: "type-2"
    field_offset: 0
    field_name: "a"
    access: public_access
  }
  access: public_access
  record_kind: class_kind
  tag_info {
    unique_id: "_ZTSN5test35OuterE"
  }
}
record_types {
  type_info {
    name: "test3::Outer::Inner"
    size: 4
    alignment: 4
    referenced_type: "type-18"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test3::Outer::Inner"
    self_type: "type-18"
  }
  fields {
    referenced_type: "type-2"
    field_offset: 0
    field_name: "b"
    access: private_access
  }
  access: private_access
  record_kind: class_kind
  tag_info {
    unique_id: "_ZTSN5test35Outer5InnerE"
  }
}
enum_types {
  type_info {
    name: "Foo_s"
    size: 4
    alignment: 4
    referenced_type: "type-8"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "Foo_s"
    self_type: "type-8"
  }
  underlying_type: "type-9"
  enum_fields {
    enum_field_value: 10
    name: "Foo_s::foosball"
  }
  enum_fields {
    enum_field_value: 11
    name: "Foo_s::foosbat"
  }
  access: public_access
  tag_info {
    unique_id: "_ZTS5Foo_s"
  }
}
enum_types {
  type_info {
    name: "test3::Kind"
    size: 4
    alignment: 4
    referenced_type: "type-16"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test3::Kind"
    self_type: "type-16"
  }
  underlying_type: "type-9"
  enum_fields {
    enum_field_value: 24
    name: "test3::Kind::kind1"
  }
  enum_fields {
    enum_field_value: 2312
    name: "test3::Kind::kind2"
  }
  access: public_access
  tag_info {
    unique_id: "_ZTSN5test34KindE"
  }
}
pointer_types {
  type_info {
    name: "test2::HelloAgain *"
    size: 8
    alignment: 8
    referenced_type: "type-4"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "test2::HelloAgain *"
    self_type: "type-7"
  }
}
builtin_types {
  type_info {
    name: "int"
    size: 4
    alignment: 4
    referenced_type: "type-2"
    source_file: ""
    linker_set_key: "int"
    self_type: "type-2"
  }
  is_unsigned: false
  is_integral: true
}
builtin_types {
  type_info {
    name: "float"
    size: 4
    alignment: 4
    referenced_type: "type-3"
    source_file: ""
    linker_set_key: "float"
    self_type: "type-3"
  }
  is_unsigned: false
  is_integral: false
}
builtin_types {
  type_info {
    name: "void"
    size: 0
    alignment: 0
    referenced_type: "type-6"
    source_file: ""
    linker_set_key: "void"
    self_type: "type-6"
  }
  is_unsigned: false
  is_integral: false
}
builtin_types {
  type_info {
    name: "unsigned int"
    size: 4
    alignment: 4
    referenced_type: "type-9"
    source_file: ""
    linker_set_key: "unsigned int"
    self_type: "type-9"
  }
  is_unsigned: true
  is_integral: true
}
builtin_types {
  type_info {
    name: "bool"
    size: 1
    alignment: 1
    referenced_type: "type-12"
    source_file: ""
    linker_set_key: "bool"
    self_type: "type-12"
  }
  is_unsigned: true
  is_integral: true
}
builtin_types {
  type_info {
    name: "double"
    size: 8
    alignment: 8
    referenced_type: "type-14"
    source_file: ""
    linker_set_key: "double"
    self_type: "type-14"
  }
  is_unsigned: false
  is_integral: false
}
qualified_types {
  type_info {
    name: "bool const[2]"
    size: 2
    alignment: 1
    referenced_type: "type-10"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "bool const[2]"
    self_type: "type-11"
  }
  is_const: true
  is_volatile: false
  is_restricted: false
}
array_types {
  type_info {
    name: "bool [2]"
    size: 2
    alignment: 1
    referenced_type: "type-12"
    source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
    linker_set_key: "bool [2]"
    self_type: "type-10"
  }
}
functions {
  return_type: "type-6"
  function_name: "test2::HelloAgain::~HelloAgain"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  parameters {
    referenced_type: "type-7"
    default_arg: false
    is_this_ptr: true
  }
  linker_set_key: "_ZN5test210HelloAgainD2Ev"
  access: public_access
}
functions {
  return_type: "type-6"
  function_name: "test2::HelloAgain::~HelloAgain"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  parameters {
    referenced_type: "type-7"
    default_arg: false
    is_this_ptr: true
  }
  linker_set_key: "_ZN5test210HelloAgainD1Ev"
  access: public_access
}
functions {
  return_type: "type-6"
  function_name: "test2::HelloAgain::~HelloAgain"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  parameters {
    referenced_type: "type-7"
    default_arg: false
    is_this_ptr: true
  }
  linker_set_key: "_ZN5test210HelloAgainD0Ev"
  access: public_access
}
functions {
  return_type: "type-12"
  function_name: "test3::End"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  parameters {
    referenced_type: "type-3"
    default_arg: true
    is_this_ptr: false
  }
  linker_set_key: "_ZN5test33EndEf"
  access: public_access
}
global_vars {
  name: "test2::HelloAgain::hello_forever"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  linker_set_key: "_ZN5test210HelloAgain13hello_foreverE"
  referenced_type: "type-2"
  access: public_access
}
global_vars {
  name: "__test_var"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  linker_set_key: "_ZL10__test_var"
  referenced_type: "type-11"
  access: public_access
}
global_vars {
  name: "test3::ByeAgain<float>::foo_forever"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  linker_set_key: "_ZN5test38ByeAgainIfE11foo_foreverE"
  referenced_type: "type-2"
  access: public_access
}
global_vars {
  name: "test3::double_bye"
  source_file: "/development/vndk/tools/header-checker/tests/input/example2.h"
  linker_set_key: "_ZN5test310double_byeE"
  referenced_type: "type-13"
  access: public_access
}
