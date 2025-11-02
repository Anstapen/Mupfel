#include "MovementSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include "glad.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "Core/Profiler.h"
#include <cassert>
#include "GPU/GPUComponentStorage.h"
#include <memory>

using namespace Mupfel;

struct ComponentIndices {
	uint32_t transform_index;
	uint32_t velocity_index;
};


static std::unique_ptr<GPUComponentStorage<ComponentIndices>> indices = nullptr;



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

	reg.SetArchetype<Transform, Velocity>();

	reg.CreateLinkedComponentArray<Mupfel::Transform>(StorageType::GPU);

	reg.CreateLinkedComponentArray<Mupfel::Velocity>(StorageType::GPU);

	indices.reset( new GPUComponentStorage<ComponentIndices>(5000));
}

void MovementSystem::DeInit()
{
}

void MovementSystem::Update(double elapsedTime)
{
	ProfilingSample prof("GPU based physics update");

	{
		ProfilingSample prof("Creating indices");
		std::vector<Entity> changed_entities;

		indices->clear();

		auto& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
		auto& velocity_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Velocity>();

		Application::GetCurrentRegistry().ParallelForEach<Mupfel::Transform, Mupfel::Velocity>(
			[](Entity ent, Mupfel::Transform t, Mupfel::Velocity vel) -> bool {
				//ComponentIndices ci = { static_cast<uint32_t>(transform_array.sparse[ent.Index()]),
										//static_cast<uint32_t>(velocity_array.sparse[ent.Index()]) };
				//indices->push_back(ci);
				return true;
			},
			changed_entities
		);

		for (auto& e : changed_entities)
		{
			ComponentIndices ci = { static_cast<uint32_t>(transform_array.sparse[e.Index()]),
									static_cast<uint32_t>(velocity_array.sparse[e.Index()]) };
			indices->push_back(std::move(ci));
		}
	}

	{
		ProfilingSample prof("Running Movement Shader");
		uint32_t entity_count = indices->size();

		if (entity_count == 0)
		{
			return;
		}

		unsigned int t_ssbo = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>().Id();
		unsigned int v_ssbo = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Velocity>().Id();

		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		glUseProgram(shader_id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, t_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, v_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indices->Id());

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
		//Application::GetCurrentEventSystem().AddImmediateEvent<MovementSystemUpdateEvent>({ kinematic_ssbo, entity_count });
	}
	
}