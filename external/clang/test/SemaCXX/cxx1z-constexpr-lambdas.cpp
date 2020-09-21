// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s 
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s 
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s 

namespace test_constexpr_checking {

namespace ns1 {
  struct NonLit { ~NonLit(); };  //expected-note{{not literal}}
  auto L = [](NonLit NL) constexpr { }; //expected-error{{not a literal type}}
} // end ns1

namespace ns2 {
  auto L = [](int I) constexpr { asm("non-constexpr");  }; //expected-error{{not allowed in constexpr function}}
} // end ns1

} // end ns test_constexpr_checking

namespace test_constexpr_call {

namespace ns1 {
  auto L = [](int I) { return I; };
  static_assert(L(3) == 3);
} // end ns1
namespace ns2 {
  auto L = [](auto a) { return a; };
  static_assert(L(3) == 3);
  static_assert(L(3.14) == 3.14);
}
namespace ns3 {
  auto L = [](auto a) { asm("non-constexpr"); return a; }; //expected-note{{declared here}}
  constexpr int I =  //expected-error{{must be initialized by a constant expression}}
      L(3); //expected-note{{non-constexpr function}}
} 

} // end ns test_constexpr_call