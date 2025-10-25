#include "MovementSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include "glad.h"
#include "glm.hpp"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "Core/GUID.h"
#include "ECS/Entity.h"
#include "Core/Profiler.h"

using namespace Mupfel;

// Struct has to be compatible to std430
struct MovementEntityGPU {
	glm::vec2 position;
	glm::vec2 velocity;
	//glm::vec2 scale;
	//glm::vec2 rotation;
};

static constexpr size_t invalid_entry = std::numeric_limits<size_t>::max();

MovementSystem::MovementSystem(uint32_t in_max_entities) :
	max_entities(in_max_entities),
	shader_id(0),
	ssbo_id(0),
	current_ssbo_index(0),
	mapped_ssbo(nullptr)
{
	sparse.resize(max_entities, invalid_entry);
}

MovementSystem::~MovementSystem()
{
	if (shader_id != 0)
	{
		glDeleteProgram(shader_id);
	}

	if (ssbo_id != 0)
	{
		glDeleteBuffers(1, &ssbo_id);
	}
}

void Mupfel::MovementSystem::Init()
{
	/* Load the Movement Compute Shader */
	char* shader_code = LoadFileText("Shaders/movement_system.glsl");
	int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Register Handlers */
	auto& evt_system = Application::GetCurrentEventSystem();
	evt_system.RegisterListener<ComponentAddedEvent>(
		[this](const ComponentAddedEvent& evt) {
			AddedComponentHandler(evt);
		}
	);

	evt_system.RegisterListener<ComponentRemovedEvent>(
		[this](const ComponentRemovedEvent& evt) {
			RemovedComponentHandler(evt);
		}
	);

	CreateOrResizeSSBO(max_entities);
	dense.reserve(max_entities);
}

void MovementSystem::DeInit()
{
}

void MovementSystem::Update(double elapsedTime)
{
	ProfilingSample prof("GPU based physics update");
	uint32_t entity_count = dense.size();
	if (entity_count == 0)
	{
		return;
	}

	glUseProgram(shader_id);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_id);

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

	/* Send an Event to notify other Systems */
	Application::GetCurrentEventSystem().AddImmediateEvent<MovementSystemUpdateEvent>({ssbo_id, current_ssbo_index});
}

void MovementSystem::UpdateEntities()
{
	/* TODO */
}

void MovementSystem::AddedComponentHandler(const ComponentAddedEvent& evt)
{
	static size_t transform_comp_id = CompUtil::GetComponentTypeID<Transform>();
	static size_t velocity_comp_id = CompUtil::GetComponentTypeID<Velocity>();

	/* We only care about Transform and Velocity components */
	if (evt.comp_id != transform_comp_id && evt.comp_id != velocity_comp_id)
	{
		return;
	}

	Entity::Signature entity_sig = evt.sig;
	static Entity::Signature wanted_sig = CompUtil::ComponentSignature<Transform, Velocity>();
	if ((entity_sig & wanted_sig) != wanted_sig)
	{
		/* Nothing to do */
		return;
	}

	/* Only add the entity, if it is not already added */
	if (sparse.size() > evt.e.Index() && sparse[evt.e.Index()] != invalid_entry)
	{
		return;
	}

	AddEntity(evt.e);
}

void Mupfel::MovementSystem::RemovedComponentHandler(const ComponentRemovedEvent& evt)
{
	static size_t transform_comp_id = CompUtil::GetComponentTypeID<Transform>();
	static size_t velocity_comp_id = CompUtil::GetComponentTypeID<Velocity>();
	/* We only care about Transform and Velocity components */
	if (evt.comp_id != transform_comp_id && evt.comp_id != velocity_comp_id)
	{
		return;
	}

	Entity::Signature entity_sig = evt.sig;
	static Entity::Signature wanted_sig = CompUtil::ComponentSignature<Transform, Velocity>();
	/* If there are changes to Velocity or Transform, we only care about still moveable Entities */
	if ((entity_sig & wanted_sig) != wanted_sig)
	{
		/* Nothing to do */
		return;
	}


	/* Only remove the entity, if it exists */
	if (sparse.size() > evt.e.Index() && sparse[evt.e.Index()] == invalid_entry)
	{
		return;
	}

	/* The Entity in question will not be moveable after the component remove,
	so remove it from the storage buffer */
	RemoveEntity(evt.e);
}

void Mupfel::MovementSystem::AddEntity(Entity e)
{
	uint32_t index = e.Index();

	/* Check if the sparse array is large enough, if not, resize it */
	if (index >= sparse.size())
	{
		sparse.resize((sparse.size() + 2) * 2, invalid_entry);
	}

	/* We expect no active Entity at the location */
	assert((sparse[index] == invalid_entry) && "Expected no Entity at this location!");

	/* Check if there is still space in the SSBO */
	if (current_ssbo_index >= max_entities)
	{
		size_t new_size = (max_entities + 1) * 2;
		CreateOrResizeSSBO(new_size);
		dense.reserve(new_size);
	}
	
	/* Get the component arrays for the transform and velocity information */
	auto& reg = Application::GetCurrentRegistry();
	Transform entity_pos = reg.GetComponentArray<Transform>().Get(e);
	Velocity entity_velocity = reg.GetComponentArray<Velocity>().Get(e);

	glm::vec2 pos{ entity_pos.pos.x, entity_pos.pos.y };
	glm::vec2 vel{ entity_velocity.x, entity_velocity.y };

	MovementEntityGPU ent = {pos, vel};

	auto ptr = static_cast<MovementEntityGPU*>(mapped_ssbo);

	ptr += current_ssbo_index;

	/* Copy the new entry into the GPU Buffer */
	std::memcpy(ptr, &ent, sizeof(MovementEntityGPU));

	/* Save the entity id for this sbbo entry */
	dense.push_back(e.Index());

	/* Enter the buffer index into the sparse array */
	sparse[index] = current_ssbo_index;

	current_ssbo_index++;

	assert(current_ssbo_index == dense.size());

	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

}

void Mupfel::MovementSystem::RemoveEntity(Entity e)
{
	uint32_t index = e.Index();
	assert(index < sparse.size() && (sparse[index] != invalid_entry) && "Expected Entity at this location!");
	assert(current_ssbo_index > 0);
	assert(current_ssbo_index == dense.size());
	assert(mapped_ssbo);
	
	uint32_t last_entry = current_ssbo_index - 1;

	uint32_t entry_to_be_removed = sparse[index];

	if (last_entry == entry_to_be_removed)
	{
		/* Simple case, just remove last element */
		current_ssbo_index--;
		dense.pop_back();
		sparse[index] = invalid_entry;
		return;
	}

	/* Do a swap remove */
	uint32_t entity_to_be_moved = dense[last_entry];

	/* Update the sparse array */
	sparse[entity_to_be_moved] = entry_to_be_removed;
	sparse[index] = invalid_entry;

	/* Update the dense array */
	dense[entry_to_be_removed] = entity_to_be_moved;
	dense.pop_back();

	/* Copy the content */
	auto to = static_cast<MovementEntityGPU*>(mapped_ssbo) + entry_to_be_removed;
	auto from = static_cast<MovementEntityGPU*>(mapped_ssbo) + last_entry;

	std::memcpy(to, from, sizeof(MovementEntityGPU));

	current_ssbo_index--;

	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

void Mupfel::MovementSystem::CreateOrResizeSSBO(uint32_t capacity)
{
	std::vector<MovementEntityGPU> temp_cpy;

	if (ssbo_id != 0)
	{
		/*
			We already have an existing buffer, but we need a larger one.
			Copy all the currently existing entities in a temporary buffer.
		*/
	}

	if (ssbo_id != 0)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id);

		auto buff = static_cast<MovementEntityGPU*>(mapped_ssbo);

		/* Copy the existing buffer */
		for (uint32_t i = 0; i < current_ssbo_index; i++)
		{
			temp_cpy.push_back(buff[i]);
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glDeleteBuffers(1, &ssbo_id);
		ssbo_id = 0;
		mapped_ssbo = nullptr;
	}

	glGenBuffers(1, &ssbo_id);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id);

	static constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(MovementEntityGPU) * capacity, nullptr, flags);

	mapped_ssbo = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(MovementEntityGPU) * capacity, flags);
	if (!mapped_ssbo)
	{
		TraceLog(LOG_ERROR, "Persistent mapping failed (realloc)!");
	}

	max_entities = capacity;

	auto buff = static_cast<MovementEntityGPU*>(mapped_ssbo);

	for (uint32_t i = 0; i < current_ssbo_index; i++)
	{
		buff[i] = temp_cpy[i];
	}
}