#include "MovementSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include "glad.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/Registry.h"
#include "Core/Profiler.h"
#include <algorithm>
#include <memory>
#include "GPU/GPUVector.h"

using namespace Mupfel;

/**
 * @brief Represents a mapping of an active entity in the Movement System.
 *
 * The ActiveEntity structure links an entity's unique ID to the
 * corresponding indices of its Transform and Velocity components
 * in their respective GPU buffers. This structure is stored in
 * a GPU buffer and is used by the movement compute shader to
 * perform position updates for each active entity.
 */
struct ActiveEntity {
	/**
	 * @brief The unique identifier of the entity.
	 *
	 * This ID corresponds to the ECS entity index and allows
	 * the compute shader to reference entities consistently
	 * across component arrays.
	 */
	uint32_t entity_id = 0;

	/**
	 * @brief The dense array index of the Transform component for this entity.
	 *
	 * Used to locate the Transform data of the entity within
	 * the Transform GPU buffer.
	 */
	uint32_t transform_index = 0;

	/**
	 * @brief The dense array index of the Velocity component for this entity.
	 *
	 * Used to locate the Velocity data of the entity within
	 * the Velocity GPU buffer.
	 */
	uint32_t velocity_index = 0;
};

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
	 * @brief Bitmask representing the required component signature.
	 *
	 * Used to filter which entities the Movement System should process.
	 * Corresponds to the combined signature of Transform
	 * and Velocity components.
	 */
	uint64_t component_mask = 0;

	/**
	 * @brief The total number of currently active entities.
	 *
	 * For now, this is equal to the number of entities that have a
	 * transform component. Might change in the future if there
	 * are more clever ways on how to store the active entity buffers.
	 */
	uint64_t active_entities = 0;

	/**
	 * @brief The number of entities added during the current frame.
	 *
	 * Used by the join compute shader to determine how many new
	 * entities need to be inserted into the active entity list.
	 */
	uint64_t entities_added = 0;

	/**
	 * @brief The number of entities deleted during the current frame.
	 *
	 * Used by the join compute shader to remove entities that have
	 * lost one or more required components.
	 */
	uint64_t entities_deleted = 0;

	/**
	 * @brief The elapsed time (in seconds) since the last frame.
	 *
	 * Passed to the movement compute shader to ensure frame-rate-independent
	 * movement calculations.
	 */
	float delta_time;
};

/**
 * @brief Holds a GPU buffer of all currently active entities that have both Transform and Velocity components.
 */
static std::unique_ptr<GPUVector<ActiveEntity>> active_entities = nullptr;

/**
 * @brief GPU buffer that stores global parameters for the movement compute shader (entity count, delta time, etc.).
 */
static std::unique_ptr<GPUVector<ProgramParams>> program_params = nullptr;

/**
 * @brief GPU buffer containing entities that were added this frame and need to be processed by the join shader.
 */
static std::unique_ptr<GPUVector<Entity>> added_entities = nullptr;

/**
 * @brief CPU-side counter of how many entities were added during the current frame.
 */
static uint32_t entities_added_this_frame = 0;

/**
 * @brief GPU buffer containing entities that lost required components and should be removed from the active list.
 */
static std::unique_ptr<GPUVector<Entity>> deleted_entities = nullptr;

/**
 * @brief CPU-side counter of how many entities were removed during the current frame.
 */
static uint32_t entities_deleted_this_frame = 0;

/**
 * @brief OpenGL Shader Storage Buffer Object that holds the current program parameters for GPU-side computation.
 */
static GLuint programParamsSSBO = 0;

/**
 * @brief Component signature mask describing which components the system requires (Transform + Velocity).
 */
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Velocity>();

/**
 * @brief OpenGL shader program ID for the main movement update compute shader.
 */
static uint32_t movement_update_shader_id = 0;

/**
 * @brief OpenGL shader program ID for the data join compute shader, used to sync active entity lists.
 */
static uint32_t join_shader_id = 0;

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

	/* Load the Movement Data Join Shader */
	shader_code = LoadFileText("Shaders/movement_data_join.glsl");
	shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	join_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Create a GPUVector for the active pairs */
	active_entities = std::make_unique<GPUVector<ActiveEntity>>();
	active_entities->resize(10000, { 0, 0, 0 });

	/* Create a GPUVector for the program parameters */
	program_params = std::make_unique<GPUVector<ProgramParams>>();
	program_params->resize(1, { static_cast<uint64_t>(wanted_comp_sig.to_ulong()), 0 , 0, 0, 0.0f});

	/* Create a GPUVector for the added entities every frame */
	added_entities = std::make_unique<GPUVector<Entity>>();
	added_entities->resize(100, {Entity()});

	/* Create a GPUVector for the deleted entities every frame */
	deleted_entities = std::make_unique<GPUVector<Entity>>();
	deleted_entities->resize(100, { Entity() });


	glCreateBuffers(1, &programParamsSSBO);
	glNamedBufferStorage(programParamsSSBO, sizeof(ProgramParams), nullptr, GL_DYNAMIC_STORAGE_BIT);

	SetEventCallbacks();

}

void MovementSystem::DeInit()
{
	/* Unload Shaders */
	rlUnloadShaderProgram(movement_update_shader_id);
	rlUnloadShaderProgram(join_shader_id);

	movement_update_shader_id = 0;
	join_shader_id = 0;

	/* Delete Program Params Buffer */
	glDeleteBuffers(1, &programParamsSSBO);
	programParamsSSBO = 0;
}

void MovementSystem::Update(double elapsedTime)
{
	SetProgramParams(elapsedTime);
	Join();
	Move(elapsedTime);
}

void Mupfel::MovementSystem::Join()
{
	ProfilingSample prof("Join");

	/* The Join Shader only needs to do work if Tranform or Velocity Components were added/deleted */
	if (entities_added_this_frame == 0 && entities_deleted_this_frame == 0)
	{
		return;
	}

	glUseProgram(join_shader_id);

	/* Get the needed buffers from the current Registry */
	uint32_t signatureBuffer = Application::GetCurrentRegistry().signatures.GetSSBOID();
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Velocity>& velocity_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Velocity>();

	/* Do we need to resize the active entity buffer? */
	if (transform_array.Size() >= active_entities->size())
	{
		/* Resize the active entity buffer */
		active_entities->resize(transform_array.Size() * 2, { 0, 0, 0 });
	}

	/* Bind the Entity Signature Array to slot 0 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, signatureBuffer);

	/* Bind the Transform Sparse Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, transform_array.GetSparseSSBO());

	/* Bind the Velocity Sparse Array to slot 4 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, velocity_array.GetSparseSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetSSBOID());

	/* Bind the Shader parameters to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	/* Bind the Added Entities Array to slot 10 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, added_entities->GetSSBOID());

	/* Bind the Deleted Entities Array to slot 11 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, deleted_entities->GetSSBOID());

	uint32_t changed_entities = std::max<uint32_t>(entities_added_this_frame, entities_deleted_this_frame);

	GLuint groups = (changed_entities + 255) / 256;
	glDispatchCompute(groups, 1, 1);

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glFinish();

	entities_added_this_frame = 0;
	entities_deleted_this_frame = 0;

}

void Mupfel::MovementSystem::Move(double elapsedTime)
{
	ProfilingSample prof("Compute");
	glUseProgram(movement_update_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Velocity>& velocity_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Velocity>();

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

	/* Bind the Velocity Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, velocity_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetSSBOID());

	/* Bind the Program Params buffer to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	GLuint moveGroups = (transform_array.Size() + 255) / 256;
	glDispatchCompute(moveGroups, 1, 1);
	glFinish();
}

void Mupfel::MovementSystem::SetProgramParams(double elapsedTime)
{
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();

	/* Update the Shader Program parameters for the GPU */
	ProgramParams params{};
	glGetNamedBufferSubData(programParamsSSBO, 0, sizeof(ProgramParams), &params);

	params.component_mask = static_cast<uint64_t>(wanted_comp_sig.to_ulong());
	params.entities_added = entities_added_this_frame;
	params.entities_deleted = entities_deleted_this_frame;
	params.active_entities = transform_array.Size();
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

			if (entities_added_this_frame >= added_entities->size())
			{
				added_entities->resize(entities_added_this_frame * 2, Entity());
			}

			added_entities->operator[](entities_added_this_frame) = event.e;

			entities_added_this_frame++;
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
			velocity_sig.set(ComponentIndex::Index<Mupfel::Velocity>());

			uint32_t has_transform_component = (event.sig & transform_sig) != 0 ? 1 : 0;
			uint32_t has_velocity_component = (event.sig & velocity_sig) != 0 ? 1 : 0;

			uint32_t comp_info = has_transform_component + has_velocity_component;

			/* We only care about the entity if it has exactly one of the needed components */
			if (comp_info != 1)
			{
				return;
			}

			/* Add the entity to the delete array */

			if (entities_deleted_this_frame >= deleted_entities->size())
			{
				deleted_entities->resize(entities_deleted_this_frame * 2, Entity());
			}

			deleted_entities->operator[](entities_deleted_this_frame) = event.e;

			entities_deleted_this_frame++;
		}
	);
}
