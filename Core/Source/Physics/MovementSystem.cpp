#include "MovementSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include "glad.h"
#include "glm.hpp"
#include "Core/Application.h"
#include "ECS/Components/Kinematic.h"
#include "Core/GUID.h"
#include "ECS/Entity.h"
#include "Core/Profiler.h"
#include <cassert>

using namespace Mupfel;

static unsigned int kinematic_ssbo = 0;


MovementSystem::MovementSystem(uint32_t in_max_entities)
{
}

MovementSystem::~MovementSystem()
{
}

void Mupfel::MovementSystem::Init()
{
	/* Load the Movement Compute Shader */
	char* shader_code = LoadFileText("Shaders/movement_system.glsl");
	int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Create the component buffers on the GPU */
	auto& reg = Application::GetCurrentRegistry();

	auto &kinematic_array = reg.CreateComponentArray<Mupfel::Kinematic>(StorageType::GPU);
	kinematic_ssbo = kinematic_array.components->Id();

}

void MovementSystem::DeInit()
{
}

void MovementSystem::Update(double elapsedTime)
{
	ProfilingSample prof("GPU based physics update");

	auto& kinematic_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Kinematic>();

	uint32_t entity_count = kinematic_array.components->size();

	if (entity_count == 0)
	{
		return;
	}

	glUseProgram(shader_id);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kinematic_ssbo);

	/* Update the uniforms */
	GLint locDt = glGetUniformLocation(shader_id, "uDeltaTime");
	GLint locCount = glGetUniformLocation(shader_id, "uEntityCount");
	if (locDt >= 0) glUniform1f(locDt, elapsedTime);
	if (locCount >= 0) glUniform1ui(locCount, entity_count);

	/* Calculate workgroup size, one thread handles 256 entities */
	GLuint groups = (entity_count + 255) / 256;
	glDispatchCompute(groups, 1, 1);

	/* Memory Barrier to make sure that the Vertex Shader sees the updated state */
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		TraceLog(LOG_ERROR, "### GPU Allocator Error!");
	}

	/* Send an Event to notify other Systems */
	Application::GetCurrentEventSystem().AddImmediateEvent<MovementSystemUpdateEvent>({ kinematic_ssbo, entity_count });
}