#include "MovementSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include "glad.h"
#include "glm.hpp"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "Core/Profiler.h"
#include <cassert>
#include <algorithm>
#include <memory>

#include "ECS/GPUComponentArray.h"
#include "GPU/GPUVector.h"

using namespace Mupfel;

struct ActiveEntity {
	uint32_t entity_id = 0;
	uint32_t transform_index = 0;
	uint32_t velocity_index = 0;
};

struct ProgramParams {
	uint64_t component_mask = 0;
	uint64_t active_entities = 0;
	uint64_t entities_added = 0;
	uint64_t entities_deleted = 0;
	float delta_time;
};

static std::unique_ptr<GPUVector<ActiveEntity>> active_entities = nullptr;

static std::unique_ptr<GPUVector<ProgramParams>> program_params = nullptr;

static std::unique_ptr<GPUVector<Entity>> added_entities = nullptr;
static uint32_t entities_added_this_frame = 0;

static std::unique_ptr<GPUVector<Entity>> deleted_entities = nullptr;
static uint32_t entities_deleted_this_frame = 0;


static GLuint frameParamsSSBO = 0;

static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Velocity>();

MovementSystem::MovementSystem(uint32_t in_max_entities) : movement_update_shader_id(0), join_shader_id(0)
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
	active_entities->resize(100000, { 0, 0, 0 });

	/* Create a GPUVector for the program parameters */
	program_params = std::make_unique<GPUVector<ProgramParams>>();
	program_params->resize(1, { static_cast<uint64_t>(wanted_comp_sig.to_ulong()), 0 , 0, 0, 0.0f});

	/* Create a GPUVector for the added entities every frame */
	added_entities = std::make_unique<GPUVector<Entity>>();
	added_entities->resize(100, {Entity()});

	/* Create a GPUVector for the deleted entities every frame */
	deleted_entities = std::make_unique<GPUVector<Entity>>();
	deleted_entities->resize(100, { Entity() });


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


	glCreateBuffers(1, &frameParamsSSBO);
	glNamedBufferStorage(frameParamsSSBO, sizeof(ProgramParams), nullptr, GL_DYNAMIC_STORAGE_BIT);

}

void MovementSystem::DeInit()
{
}

void MovementSystem::Update(double elapsedTime)
{
	JoinAndMovement(elapsedTime);
}


void Mupfel::MovementSystem::JoinAndMovement(double elapsedTime)
{
	glUseProgram(join_shader_id);

	uint32_t signatureBuffer = Application::GetCurrentRegistry().signatures.GetSSBOID();

	GPUComponentArray<Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Transform>();
	GPUComponentArray<Velocity>& velocity_array = Application::GetCurrentRegistry().GetComponentArray<Velocity>();

	ProgramParams params{};
	glGetNamedBufferSubData(frameParamsSSBO, 0, sizeof(ProgramParams), &params);
	params.delta_time = static_cast<float>(elapsedTime);
	params.active_entities = transform_array.GetDenseSize();
	glNamedBufferSubData(frameParamsSSBO, 0, sizeof(ProgramParams), &params);


	/* Bind SSBOs that are needed by both shaders */
	/* Bind the Transform Component Array to slot 3 */
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetSSBO(),
		transform_array.GetComponentOffsetInBytes(),
		transform_array.GetSizeInBytes() - transform_array.GetComponentOffsetInBytes());

	/* Bind the Velocity Component Array to slot 6 */
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 6, velocity_array.GetSSBO(),
		velocity_array.GetComponentOffsetInBytes(),
		velocity_array.GetSizeInBytes() - velocity_array.GetComponentOffsetInBytes());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetSSBOID());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, frameParamsSSBO);

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	{
		ProfilingSample prof("Join");

		if (entities_added_this_frame > 0 || entities_deleted_this_frame > 0)
		{
			if (transform_array.GetDenseSize() >= active_entities->size())
			{
				/* Resize the active entity buffer */
				active_entities->resize(transform_array.GetDenseSize() * 2, { 0, 0, 0 });
			}

			ProgramParams params{};
			glGetNamedBufferSubData(frameParamsSSBO, 0, sizeof(ProgramParams), &params);

			params.entities_added = entities_added_this_frame;
			params.entities_deleted = entities_deleted_this_frame;

			glNamedBufferSubData(frameParamsSSBO, 0, sizeof(ProgramParams), &params);

			/* Bind the Entity Signature Array to slot 0 */
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, signatureBuffer);

			/* Bind the Added Entities Array to slot 10 */
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, added_entities->GetSSBOID());

			/* Bind the Deleted Entities Array to slot 11 */
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, deleted_entities->GetSSBOID());

			/* Bind the Transform Sparse Array to slot 1 */
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, transform_array.GetSSBO(),
				0, transform_array.GetDenseOffsetInBytes());

			/* Bind the Transform Dense Array to slot 2 */
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, transform_array.GetSSBO(),
				transform_array.GetDenseOffsetInBytes(),
				transform_array.GetComponentOffsetInBytes() - transform_array.GetDenseOffsetInBytes());

			/* Bind the Velocity Sparse Array to slot 4 */
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 4, velocity_array.GetSSBO(),
				0, velocity_array.GetDenseOffsetInBytes());

			/* Bind the Velocity Dense Array to slot 5 */
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 5, velocity_array.GetSSBO(),
				velocity_array.GetDenseOffsetInBytes(),
				velocity_array.GetComponentOffsetInBytes() - velocity_array.GetDenseOffsetInBytes());

			uint32_t changed_entities = std::max<uint32_t>(entities_added_this_frame, entities_deleted_this_frame);

			GLuint groups = (changed_entities + 255) / 256;
			glDispatchCompute(groups, 1, 1);

			//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glFinish();

			entities_added_this_frame = 0;
			entities_deleted_this_frame = 0;
		}
		
	}

	{
		ProfilingSample prof("Compute");
		glUseProgram(movement_update_shader_id);

		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		GLuint moveGroups = (transform_array.GetDenseSize() + 255) / 256;
		glDispatchCompute(moveGroups, 1, 1);
		glFinish();
	}
	/* We don't care about memory synchronisation here. */
}
