; RUN: llvm-as < %s | llvm-spirv -spirv-text -o %t
; RUN: FileCheck < %t %s
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir-unknown-unknown"

; CHECK: 4 Decorate [[GID:[0-9]*]] FuncParamAttr 5
; CHECK: 2 DecorationGroup [[GID]]
; CHECK: 4 Decorate [[GID2:[0-9]*]] FuncParamAttr 6
; CHECK: 2 DecorationGroup [[GID2]]
; CHECK: 5 GroupDecorate [[GID]]
; CHECK: 4 GroupDecorate [[GID2]]

; Function Attrs: nounwind readnone
define spir_kernel void @test(<4 x i8> addrspace(1)* nocapture %src1, <4 x i8> addrspace(1)* nocapture %src2, <4 x i8> addrspace(1)* nocapture %dst) #0 {
entry:
  ret void
}

attributes #0 = { nounwind readnone }

!opencl.kernels = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!6}
!opencl.used.extensions = !{!7}
!opencl.used.optional.core.features = !{!7}
!opencl.compiler.options = !{!8}

!0 = !{void (<4 x i8> addrspace(1)*, <4 x i8> addrspace(1)*, <4 x i8> addrspace(1)*)* @test, !1, !2, !3, !4, !5}
!1 = !{!"kernel_arg_addr_space", i32 1, i32 1, i32 1}
!2 = !{!"kernel_arg_access_qual", !"none", !"none", !"none"}
!3 = !{!"kernel_arg_type", !"char4*", !"char4*", !"char4*"}
!4 = !{!"kernel_arg_type_qual", !"const", !"const", !""}
!5 = !{!"kernel_arg_base_type", !"char4*", !"char4*", !"char4*"}
!6 = !{i32 1, i32 2}
!7 = !{}
!8 = !{!""}
