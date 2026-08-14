set SLANGC=C:/VulkanSDK/1.4.341.1/Bin/slangc.exe
set CAPS=spvShaderNonUniformEXT+spvImageQuery+spvImageGatherExtended+spvSparseResidency+spvMinLod+spvDerivativeControl+spvFragmentFullyCoveredEXT+SPV_KHR_non_semantic_info+SPV_GOOGLE_user_type

set FLAGS=-target spirv -profile spirv_1_4 -capability %CAPS% -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain

%SLANGC% ecs.slang %FLAGS% -o ecs.spv
%SLANGC% imgui.slang %FLAGS% -o imgui.spv
%SLANGC% line.slang %FLAGS% -o line.spv
%SLANGC% im.slang %FLAGS% -o im.spv
%SLANGC% geo.slang %FLAGS% -o geo.spv
pause