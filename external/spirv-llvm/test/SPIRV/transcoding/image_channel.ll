; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -spirv-text -o %t.txt
; RUN: FileCheck < %t.txt %s --check-prefix=CHECK-SPIRV
; RUN: llvm-spirv %t.bc -o %t.spv
; RUN: llvm-spirv -r %t.spv -o %t.bc
; RUN: llvm-dis < %t.bc | FileCheck %s --check-prefix=CHECK-LLVM


target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir-unknown-unknown"

%opencl.image2d_t = type opaque
; CHECK-SPIRV-DAG: 4 Constant {{[0-9]+}} [[DataTypeOffsetId:[0-9]+]] 4304
; CHECK-SPIRV-DAG: 4 Constant {{[0-9]+}} [[OrderOffsetId:[0-9]+]] 4272 

; Function Attrs: nounwind
define spir_kernel void @f(%opencl.image2d_t addrspace(1)* %img, i32 addrspace(1)* nocapture %type, i32 addrspace(1)* nocapture %order) #0 {
; CHECK-LLVM-DAG: [[DTCALL:%.+]] ={{.*}} call spir_func i32 @_Z27get_image_channel_data_type11ocl_image2d
; CHECK-LLVM: [[DTSUB:%.+]] = sub i32 [[DTCALL]], 4304
; CHECK-LLVM: [[DTADD:%.+]] = add i32 [[DTSUB]], 4304
; CHECK-LLVM: store i32 [[DTADD]]
; CHECK-SPIRV: 4 ImageQueryFormat {{[0-9]+}} [[DataTypeID:[0-9]+]]
; CHECK-SPIRV: 5 IAdd {{[0-9]+}} [[DTAddID:[0-9]+]] [[DataTypeID]] [[DataTypeOffsetId]]
; CHECK-SPIRV: 5 Store {{[0-9]+}} [[DTAddID]]
  %1 = tail call spir_func i32 @_Z27get_image_channel_data_type11ocl_image2d(%opencl.image2d_t addrspace(1)* %img) #2
  store i32 %1, i32 addrspace(1)* %type, align 4
; CHECK-LLVM-DAG: [[OCALL:%.+]] ={{.*}} call spir_func i32 @_Z23get_image_channel_order11ocl_image2d
; CHECK-LLVM: [[OSUB:%.+]] = sub i32 [[OCALL]], 4272
; CHECK-LLVM: [[OADD:%.+]] = add i32 [[OSUB]], 4272
; CHECK-LLVM: store i32 [[OADD]]
; CHECK-SPIRV: 4 ImageQueryOrder {{[0-9]+}} [[OrderID:[0-9]+]]
; CHECK-SPIRV: 5 IAdd {{[0-9]+}} [[OrderAddID:[0-9]+]] [[OrderID]] [[OrderOffsetId]]
; CHECK-SPIRV: 5 Store {{[0-9]+}} [[OrderAddID]]
  %2 = tail call spir_func i32 @_Z23get_image_channel_order11ocl_image2d(%opencl.image2d_t addrspace(1)* %img) #2
  store i32 %2, i32 addrspace(1)* %order, align 4
  ret void
}

declare spir_func i32 @_Z27get_image_channel_data_type11ocl_image2d(%opencl.image2d_t addrspace(1)*) #1

declare spir_func i32 @_Z23get_image_channel_order11ocl_image2d(%opencl.image2d_t addrspace(1)*) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!opencl.kernels = !{!0}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!9}
!opencl.compiler.options = !{!8}

!0 = !{void (%opencl.image2d_t addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*)* @f, !1, !2, !3, !4, !5}
!1 = !{!"kernel_arg_addr_space", i32 1, i32 1, i32 1}
!2 = !{!"kernel_arg_access_qual", !"read_only", !"none", !"none"}
!3 = !{!"kernel_arg_type", !"image2d_t", !"int*", !"int*"}
!4 = !{!"kernel_arg_base_type", !"image2d_t", !"int*", !"int*"}
!5 = !{!"kernel_arg_type_qual", !"", !"", !""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{}
!9 = !{!"cl_images"}
