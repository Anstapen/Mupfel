set SLANGC=C:/VulkanSDK/1.4.357.0/Bin/slangc.exe
set CAPS=spvShaderNonUniformEXT+spvImageQuery+spvImageGatherExtended+spvSparseResidency+spvMinLod+spvDerivativeControl+spvFragmentFullyCoveredEXT+SPV_KHR_non_semantic_info+SPV_GOOGLE_user_type

set FLAGS=-target spirv -profile spirv_1_3 -capability %CAPS% -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain

%SLANGC% default_vertex.slang -target spirv -profile spirv_1_3 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -source-embed-style u8 -source-embed-name defaultVertex -o default_vertex.h

%SLANGC% default_vertex.slang -target spirv -profile spirv_1_3 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -o default_vertex.spv

%SLANGC% default_fragment.slang -target spirv -profile spirv_1_3 -emit-spirv-directly -fvk-use-entrypoint-name -entry fragMain -source-embed-style u8 -source-embed-name defaultFragment -o default_fragment.h

%SLANGC% default_fragment.slang -target spirv -profile spirv_1_3 -emit-spirv-directly -fvk-use-entrypoint-name -entry fragMain -o default_fragment.spv

%SLANGC% ecs.slang %FLAGS% -o ecs.spv
%SLANGC% imgui.slang %FLAGS% -o imgui.spv
%SLANGC% line.slang %FLAGS% -o line.spv
%SLANGC% im.slang %FLAGS% -o im.spv
%SLANGC% geo.slang %FLAGS% -o geo.spv
pause