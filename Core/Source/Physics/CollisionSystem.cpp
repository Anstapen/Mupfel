#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>
#include <array>
#include <thread>
#include "Core/Application.h"
#include "Renderer/Rectangle.h"
#include "CollisionProcessor.h"

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

/* Raylib/OpenGL */
#include "glad.h"
#include "raylib.h"
#include "rlgl.h"
#include "glm.hpp"

#include <iostream>

using namespace Mupfel;

/**
 * @brief Contains per-frame parameters for the Collision and Join Compute Shaders.
 *
 * The ProgramParams structure is stored in a dedicated GPU buffer
 * and holds runtime information required by both the join and
 * collision compute shaders — such as the active entity count,
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

	uint32_t cell_size_pow;

	uint32_t num_cells_x;

	uint32_t num_cells_y;

	uint32_t _padding;
};

/**
 * @brief OpenGL Shader Storage Buffer Object that holds the current program parameters for GPU-side computation.
 */
static GLuint programParamsSSBO = 0;

/**
 * @brief Component signature mask describing which components the system requires (Transform + Velocity).
 */
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Collider>();


static const uint32_t max_colliding_entities = 20000;


Mupfel::CollisionSystem::CollisionSystem(Registry& reg, EventSystem& evt_sys) :
	registry(reg),
	evt_system(evt_sys),
	collision_grid(),
	active_entities(nullptr),
	colliding_entities(nullptr),
	num_colliding_entities(nullptr),
	fill_cell_count_shader_id(0),
	fill_cell_entity_shader_id(0),
	narrow_phase_shader_id(0)
{
}
void Mupfel::CollisionSystem::SetCellSizePow(uint32_t cell_size_pow)
{
	collision_grid.SetCellSizePow(cell_size_pow);
}

void Mupfel::CollisionSystem::SetNumCells(uint32_t num_cells_x, uint32_t num_cells_y)
{
	collision_grid.SetNumCells(num_cells_x, num_cells_y);
}

void CollisionSystem::Init()
{
	prefix_sum.Init(collision_grid.GetNumCellsX() * collision_grid.GetNumCellsY());

	/* Load the Cell Update Compute Shader */
	char* shader_code = LoadFileText("Shaders/fill_cell_count.glsl");
	int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	fill_cell_count_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Load the Narrow Phase Compute Shader */
	shader_code = LoadFileText("Shaders/gpu_narrow.glsl");
	shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	narrow_phase_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Load the Narrow Phase Compute Shader */
	shader_code = LoadFileText("Shaders/fill_cell_entity_array.glsl");
	shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	fill_cell_entity_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Create a GPUVector for the active pairs */
	active_entities = std::make_unique<GPUComponentArray<uint32_t>>();

	/* Create a GPUVector for the colliding entities every frame */
	colliding_entities = std::make_unique<GPUVector<CollisionPair>>();
	colliding_entities->resize(max_colliding_entities, { CollisionPair()});

	num_colliding_entities = std::make_unique<GPUVector<uint32_t>>();
	num_colliding_entities->resize(1, { 0 });

	/* Init the Collision Grid */
	collision_grid.Init();

	glCreateBuffers(1, &programParamsSSBO);
	glNamedBufferStorage(programParamsSSBO, sizeof(ProgramParams), nullptr, GL_DYNAMIC_STORAGE_BIT);

	SetCallbacks();
}

void CollisionSystem::Update()
{
	SetProgramParams();
	ClearBuffers();
	UpdateCellCount();
	FillCellEntityArray();
	GPUNarrowPhase();
	CheckCollisions();
}


uint32_t Mupfel::CollisionSystem::WorldtoCell(Coordinate<uint32_t> c)
{
	uint32_t cell_x = c.x >> collision_grid.GetCellSizePow();
	uint32_t cell_y = c.y >> collision_grid.GetCellSizePow();

	cell_x = std::min(cell_x, collision_grid.GetNumCellsX() - 1);
	cell_y = std::min(cell_y, collision_grid.GetNumCellsY() - 1);

	return cell_y * collision_grid.GetNumCellsX() + cell_x;
}

void Mupfel::CollisionSystem::SetProgramParams()
{
	/* Update the Shader Program parameters for the GPU */
	ProgramParams params{};

	params.active_entities = active_entities->Size();
	params.cell_size_pow = collision_grid.GetCellSizePow();
	params.num_cells_x = collision_grid.GetNumCellsX();
	params.num_cells_y = collision_grid.GetNumCellsY();

	glNamedBufferSubData(programParamsSSBO, 0, sizeof(ProgramParams), &params);
}

void Mupfel::CollisionSystem::UpdateCellCount()
{
	glUseProgram(fill_cell_count_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Collider>& spatial_info_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Collider>();

	/* Bind the Collision Grid Cell Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, collision_grid.GetCellCountArraySSBO());

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, transform_array.GetSparseSSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, spatial_info_array.GetSparseSSBO());

	/* Bind the Spatial Info Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spatial_info_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetComponentSSBO());

	/* Bind the Shader parameters to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	GLuint groups = (active_entities->Size() + 255) / 256;
	glDispatchCompute(groups, 1, 1);
	//glFinish();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

	prefix_sum.Run(collision_grid.GetCellCountArray(), collision_grid.GetNumCellsX() * collision_grid.GetNumCellsY());
	//glFinish();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	/*
		The cell_count array buffer now holds the starting indices for the cells.
		As we need them for the narrow phase, we copy the indices.
	*/
	glBindBuffer(GL_COPY_READ_BUFFER, collision_grid.GetCellCountArraySSBO());
	glBindBuffer(GL_COPY_WRITE_BUFFER, collision_grid.GetCellCountIndicesSSBO());

	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, collision_grid.GetCellCountArray().size() * sizeof(uint32_t));

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

void Mupfel::CollisionSystem::FillCellEntityArray()
{
	glUseProgram(fill_cell_entity_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Collider>& spatial_info_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Collider>();

	/* Bind the Collision Grid Cell Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, collision_grid.GetCellCountArraySSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, collision_grid.GetCellEntityArraySSBO());

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, transform_array.GetSparseSSBO());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, spatial_info_array.GetSparseSSBO());

	/* Bind the Spatial Info Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spatial_info_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetComponentSSBO());

	/* Bind the Shader parameters to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	GLuint groups = (active_entities->Size() + 255) / 256;
	glDispatchCompute(groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	//glFinish();
}

void Mupfel::CollisionSystem::GPUNarrowPhase()
{
	glUseProgram(narrow_phase_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Collider>& spatial_info_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Collider>();

	/* Bind the Collision Grid Cell Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, collision_grid.GetCellCountIndicesSSBO());

	/* Bind the Collision Grid Entity Array to slot 2 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, collision_grid.GetCellEntityArraySSBO());

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetSparseSSBO());

	/* Bind the Transform Component Array to slot 4 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, transform_array.GetComponentSSBO());

	/* Bind the Spatial Info Sparse Array to slot 5 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, spatial_info_array.GetSparseSSBO());

	/* Bind the Spatial Info Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spatial_info_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetComponentSSBO());

	/* Bind the Shader parameters to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	/* Bind the Colliding Entities Array to slot 9 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, colliding_entities->GetSSBOID());

	/* Bind buffer for the number of colliding entities to slot 10 */
	num_colliding_entities->operator[](0) = 0;
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, num_colliding_entities->GetSSBOID());

	GLuint groups = ((collision_grid.GetNumCellsX() * collision_grid.GetNumCellsY()) + 255) / 256;
	glDispatchCompute(groups, 1, 1);
	glFinish();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

void Mupfel::CollisionSystem::CheckCollisions()
{
	uint32_t num_colliding = num_colliding_entities->operator[](0);
	if (num_colliding > 0)
	{
		/* Iterate through the colliding entities */

		for (uint32_t i = 0; i < num_colliding; i++)
		{

			Entity a = colliding_entities->operator[](i).entity_a;
			Entity b = colliding_entities->operator[](i).entity_b;

			/* The CollisionProcessor handles Detection and Resolution of Entities */
			CollisionProcessor::DetectAndResolve(a, b);
		}
		
	}
}

void Mupfel::CollisionSystem::ClearBuffers()
{
	GLuint zero = 0;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, collision_grid.GetCellCountArraySSBO());
	glClearBufferData(
		GL_SHADER_STORAGE_BUFFER,
		GL_R32UI,
		GL_RED_INTEGER,
		GL_UNSIGNED_INT,
		&zero
	);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Mupfel::CollisionSystem::SetCallbacks()
{
	Application::GetCurrentEventSystem().RegisterListener<ComponentAddedEvent>(
		[this](const ComponentAddedEvent& event)
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
				return;
			}

			active_entities->Insert(event.e, { event.e.Index()});
		}
	);

	Application::GetCurrentEventSystem().RegisterListener<ComponentRemovedEvent>(
		[this](const ComponentRemovedEvent& event)
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

			Entity::Signature spatial_info_sig;
			spatial_info_sig.set(ComponentIndex::Index<Mupfel::Collider>());

			uint32_t has_transform_component = (event.sig & transform_sig) != 0 ? 1 : 0;
			uint32_t has_spatial_component = (event.sig & spatial_info_sig) != 0 ? 1 : 0;

			uint32_t comp_info = has_transform_component + has_spatial_component;

			/* We only care about the entity if it has exactly one of the needed components */
			if (comp_info != 1)
			{
				return;
			}

			/* Add the entity to the delete array */
			active_entities->Remove(event.e);
		}
	);
}
