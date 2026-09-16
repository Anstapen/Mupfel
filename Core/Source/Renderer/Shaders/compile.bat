set SLANGC=C:/VulkanSDK/1.4.357.0/Bin/slangc.exe
set CAPS=spvShaderNonUniformEXT+spvImageQuery+spvImageGatherExtended+spvSparseResidency+spvMinLod+spvDerivativeControl+spvFragmentFullyCoveredEXT+SPV_KHR_non_semantic_info+SPV_GOOGLE_user_type

set FLAGS=-target spirv -profile spirv_1_3 -capability %CAPS% -emit-spirv-directly -fvk-use-entrypoint-name

%SLANGC% ecs_fragment.slang %FLAGS% -entry fragMain -source-embed-style u8 -source-embed-name ecsFragment -o ecs_fragment.h

%SLANGC% ecs_fragment.slang %FLAGS% -entry fragMain -o ecs_fragment.spv

%SLANGC% ecs_vertex.slang %FLAGS% -entry vertMain -source-embed-style u8 -source-embed-name ecsVertex -o ecs_vertex.h

%SLANGC% ecs_vertex.slang %FLAGS% -entry vertMain -o ecs_vertex.spv


%SLANGC% im_fragment.slang %FLAGS% -entry fragMain -source-embed-style u8 -source-embed-name imFragment -o im_fragment.h

%SLANGC% im_fragment.slang %FLAGS% -entry fragMain -o im_fragment.spv

%SLANGC% im_vertex.slang %FLAGS% -entry vertMain -source-embed-style u8 -source-embed-name imVertex -o im_vertex.h

%SLANGC% im_vertex.slang %FLAGS% -entry vertMain -o im_vertex.spv


%SLANGC% geo_fragment.slang %FLAGS% -entry fragMain -source-embed-style u8 -source-embed-name geoFragment -o geo_fragment.h

%SLANGC% geo_fragment.slang %FLAGS% -entry fragMain -o geo_fragment.spv

%SLANGC% geo_vertex.slang %FLAGS% -entry vertMain -source-embed-style u8 -source-embed-name geoVertex -o geo_vertex.h

%SLANGC% geo_vertex.slang %FLAGS% -entry vertMain -o geo_vertex.spv


%SLANGC% imgui_fragment.slang %FLAGS% -entry fragMain -source-embed-style u8 -source-embed-name imguiFragment -o imgui_fragment.h

%SLANGC% imgui_fragment.slang %FLAGS% -entry fragMain -o imgui_fragment.spv

%SLANGC% imgui_vertex.slang %FLAGS% -entry vertMain -source-embed-style u8 -source-embed-name imguiVertex -o imgui_vertex.h

%SLANGC% imgui_vertex.slang %FLAGS% -entry vertMain -o imgui_vertex.spv
pause