; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -spirv-text -o %t.txt
; RUN: FileCheck < %t.txt %s --check-prefix=CHECK-SPIRV
; RUN: llvm-spirv %t.bc -o %t.spv
; RUN: llvm-spirv -r %t.spv -o %t.rev.bc
; RUN: llvm-dis < %t.rev.bc | FileCheck %s --check-prefix=CHECK-LLVM

; CHECK-LLVM: call spir_func float @_Z11read_imagef16ocl_image2ddepth11ocl_samplerDv2_i(%opencl.image2d_depth_t

; CHECK-SPIRV-DAG: 7 ImageSampleExplicitLod [[RetType:[0-9]+]] [[RetID:[0-9]+]] {{[0-9]+}} {{[0-9]+}} 2 {{[0-9]+}}
; CHECK-SPIRV-DAG: 4 TypeVector [[RetType]] {{[0-9]+}} 4
; CHECK-SPIRV: 5 CompositeExtract {{[0-9]+}} {{[0-9]+}} [[RetID]] 0

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir-unknown-unknown"

%opencl.image2d_depth_t = type opaque

; Function Attrs: nounwind
define spir_kernel void @sample_kernel(%opencl.image2d_depth_t addrspace(1)* %input, i32 %imageSampler, float addrspace(1)* %xOffsets, float addrspace(1)* %yOffsets, float addrspace(1)* %results) #0 {
entry:
  %call = call spir_func i32 @_Z13get_global_idj(i32 0) #1
  %call1 = call spir_func i32 @_Z13get_global_idj(i32 1) #1
  %call2.tmp1 = call spir_func <2 x i32> @_Z13get_image_dim16ocl_image2ddepth(%opencl.image2d_depth_t addrspace(1)* %input)
  %call2.old = extractelement <2 x i32> %call2.tmp1, i32 0
  %mul = mul i32 %call1, %call2.old
  %add = add i32 %mul, %call
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %xOffsets, i32 %add
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %conv = fptosi float %0 to i32
  %vecinit = insertelement <2 x i32> undef, i32 %conv, i32 0
  %arrayidx3 = getelementptr inbounds float, float addrspace(1)* %yOffsets, i32 %add
  %1 = load float, float addrspace(1)* %arrayidx3, align 4
  %conv4 = fptosi float %1 to i32
  %vecinit5 = insertelement <2 x i32> %vecinit, i32 %conv4, i32 1
  %call6.tmp.tmp = call spir_func float @_Z11read_imagef16ocl_image2ddepth11ocl_samplerDv2_i(%opencl.image2d_depth_t addrspace(1)* %input, i32 %imageSampler, <2 x i32> %vecinit5)
  %arrayidx7 = getelementptr inbounds float, float addrspace(1)* %results, i32 %add
  store float %call6.tmp.tmp, float addrspace(1)* %arrayidx7, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func float @_Z11read_imagef16ocl_image2ddepth11ocl_samplerDv2_i(%opencl.image2d_depth_t addrspace(1)*, i32, <2 x i32>) #0

; Function Attrs: nounwind readnone
declare spir_func i32 @_Z13get_global_idj(i32) #1

; Function Attrs: nounwind
declare spir_func <2 x i32> @_Z13get_image_dim16ocl_image2ddepth(%opencl.image2d_depth_t addrspace(1)*) #0

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }

!opencl.kernels = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!6}
!opencl.used.extensions = !{!7}
!opencl.used.optional.core.features = !{!8}

!0 = !{void (%opencl.image2d_depth_t addrspace(1)*, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @sample_kernel, !1, !2, !3, !4, !5}
!1 = !{!"kernel_arg_addr_space", i32 1, i32 0, i32 1, i32 1, i32 1}
!2 = !{!"kernel_arg_access_qual", !"read_only", !"none", !"none", !"none", !"none"}
!3 = !{!"kernel_arg_type", !"image2d_depth_t", !"sampler_t", !"float*", !"float*", !"float*"}
!4 = !{!"kernel_arg_type_qual", !"", !"", !"", !"", !""}
!5 = !{!"kernel_arg_base_type", !"image2d_depth_t", !"sampler_t", !"float*", !"float*", !"float*"}
!6 = !{i32 2, i32 0}
!7 = !{!"cl_khr_depth_images"}
!8 = !{!"cl_images"}
