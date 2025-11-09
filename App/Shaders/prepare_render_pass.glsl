#version 460
#extension GL_ARB_gpu_shader_int64 : require
layout(local_size_x = 1) in;

// Indirect-Command als SSBO binden (gleicher Buffer, anderer Target später)
struct DrawElementsIndirectCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

struct ProgramParams {
	uint64_t component_mask;
	uint64_t active_entities;
	uint64_t entities_added;
    uint64_t entities_deleted;
	float delta_time;
};

layout(std430, binding = 9) buffer IndirectCmd {
    DrawElementsIndirectCommand cmd;
};

layout(std430, binding = 8) readonly buffer ProgramParam {
    ProgramParams params;
};

void main() {
    // ein Thread reicht
    cmd.instanceCount = uint(params.active_entities);  // <-- hier die Magie
    // optional defensiv (falls du es nicht im Init fixierst):
    cmd.count        = 6u;
    cmd.firstIndex   = 0u;
    cmd.baseVertex   = 0u;
    cmd.baseInstance = 0u;
}