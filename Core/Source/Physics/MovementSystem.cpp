#include "MovementSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include "glad.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "ECS/Registry.h"
#include "Core/Profiler.h"
#include <algorithm>
#include <memory>
#include "GPU/GPUVector.h"

#include <iostream>

using namespace Mupfel;

/**
 * @brief Contains per-frame parameters for the Movement and Join Compute Shaders.
 *
 * The ProgramParams structure is stored in a dedicated GPU buffer
 * and holds runtime information required by both the join and
 * movement compute shaders — such as the active entity count,
 * elapsed frame time, and information about component changes.
 *
 * The CPU updates this buffer every frame before dispatching the
 * compute shaders.
 */
struct ProgramParams {
	/**
	 * @brief The total number of currently active entities.
	 *
	 * For now, this is equal to the number of entities that have a
	 * transform component. Might change in the future if there
	 * are more clever ways on how to store the active entity buffers.
	 */
	uint64_t active_entities = 0;

	/**
	 * @brief The elapsed time (in seconds) since the last frame.
	 *
	 * Passed to the movement compute shader to ensure frame-rate-independent
	 * movement calculations.
	 */
	float delta_time;
	float _padding = 0.0f;
};

/**
 * @brief Holds a GPU buffer of all currently active entities that have both Transform and Velocity components.
 */
static std::unique_ptr<GPUComponentArray<uint32_t>> active_entities = nullptr;

/**
 * @brief OpenGL Shader Storage Buffer Object that holds the current program parameters for GPU-side computation.
 */
static GLuint programParamsSSBO = 0;

/**
 * @brief Component signature mask describing which components the system requires (Transform + Velocity).
 */
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Movement>();

/**
 * @brief OpenGL shader program ID for the main movement update compute shader.
 */
static uint32_t movement_update_shader_id = 0;

MovementSystem::MovementSystem()
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
	movement_update_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Create a GPUVector for the active pairs */
	active_entities = std::make_unique<GPUComponentArray<uint32_t>>();

	glCreateBuffers(1, &programParamsSSBO);
	glNamedBufferStorage(programParamsSSBO, sizeof(ProgramParams), nullptr, GL_DYNAMIC_STORAGE_BIT);

	SetEventCallbacks();

}

void MovementSystem::DeInit()
{
	/* Unload Shaders */
	rlUnloadShaderProgram(movement_update_shader_id);

	movement_update_shader_id = 0;

	/* Delete Program Params Buffer */
	glDeleteBuffers(1, &programParamsSSBO);
	programParamsSSBO = 0;
}

void MovementSystem::Update(double elapsedTime)
{
	SetProgramParams(elapsedTime);
	Move(elapsedTime);
}

void Mupfel::MovementSystem::Move(double elapsedTime)
{
	ProfilingSample prof("Compute");
	glUseProgram(movement_update_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Movement>& movement_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Movement>();

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, transform_array.GetSparseSSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, movement_array.GetSparseSSBO());

	/* Bind the Velocity Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, movement_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetComponentSSBO());

	/* Bind the Program Params buffer to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	GLuint moveGroups = (active_entities->Size() + 255) / 256;
	glDispatchCompute(moveGroups, 1, 1);
	glFinish();
}

void Mupfel::MovementSystem::SetProgramParams(double elapsedTime)
{

	/* Update the Shader Program parameters for the GPU */
	ProgramParams params{};

	params.active_entities = active_entities->Size();
	params.delta_time = elapsedTime;

	glNamedBufferSubData(programParamsSSBO, 0, sizeof(ProgramParams), &params);
}

void Mupfel::MovementSystem::SetEventCallbacks()
{
	Application::GetCurrentEventSystem().RegisterListener<ComponentAddedEvent>(
		[](const ComponentAddedEvent& event)
		{
			Entity::Signature test;
			test.set(event.comp_id);
			/* Check if even care about the entity */
			if ((test & wanted_comp_sig) == 0)
			{
				return;
			}

			if ((event.sig & wanted_comp_sig) != wanted_comp_sig)
			{
				/* Entity does not have Transform + Velocity */
				return;
			}
			active_entities->Insert(event.e, event.e.Index());
		}
	);

	Application::GetCurrentEventSystem().RegisterListener<ComponentRemovedEvent>(
		[](const ComponentRemovedEvent& event)
		{
			Entity::Signature test;
			test.set(event.comp_id);
			/* Check if even care about the entity */
			if ((test & wanted_comp_sig) == 0)
			{
				return;
			}

			Entity::Signature transform_sig;
			transform_sig.set(ComponentIndex::Index<Mupfel::Transform>());

			Entity::Signature velocity_sig;
			velocity_sig.set(ComponentIndex::Index<Mupfel::Movement>());

			uint32_t has_transform_component = (event.sig & transform_sig) != 0 ? 1 : 0;
			uint32_t has_velocity_component = (event.sig & velocity_sig) != 0 ? 1 : 0;

			uint32_t comp_info = has_transform_component + has_velocity_component;

			/* We only care about the entity if it has exactly one of the needed components */
			if (comp_info != 1)
			{
				return;
			}

			active_entities->Remove(event.e);
		}
	);
}
