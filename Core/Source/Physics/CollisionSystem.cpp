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

using namespace Mupfel;

/**
 * @brief Represents a mapping of an active entity in the Collision System.
 *
 * The ActiveEntity structure links an entity's unique ID to the
 * corresponding index of its Transform component in its respective
 * GPU buffer.
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


	uint32_t spatial_info_index = 0;
};

struct CollisionPair {
	uint32_t entity_a = 0;
	uint32_t entity_b = 0;
};

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

	uint32_t cell_size_pow;

	uint32_t num_cells_x;

	uint32_t num_cells_y;

	uint32_t entities_per_cell;

	uint32_t max_colliding_entities;
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
 * @brief GPU buffer containing entities that may be colliding this frame. They will be processed by the CPU later.
 */
static std::unique_ptr<GPUVector<CollisionPair>> colliding_entities = nullptr;

/**
 * @brief GPU buffer containing the number of pairs of entities that may be colliding.
 */
static std::unique_ptr<GPUVector<uint32_t>> num_colliding_entities = nullptr;

/**
 * @brief Component signature mask describing which components the system requires (Transform + Velocity).
 */
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Collider>();


static const uint32_t max_colliding_entities = 20000;

/**
 * @brief OpenGL shader program ID for the main movement update compute shader.
 */
static uint32_t cell_update_shader_id = 0;

/**
 * @brief OpenGL shader program ID for the data join compute shader, used to sync active entity lists.
 */
static uint32_t join_shader_id = 0;

/**
 * @brief OpenGL shader program ID for the narrow_phase shader, used to detect possible collisions using AABB bounding boxes.
 */
static uint32_t narrow_phase_shader_id = 0;

Mupfel::CollisionSystem::CollisionSystem(Registry& reg, EventSystem& evt_sys) :
	registry(reg),
	evt_system(evt_sys),
	collision_grid()
{
}

void CollisionSystem::Init()
{
	/* Load the Cell Update Compute Shader */
	char* shader_code = LoadFileText("Shaders/cell_update.glsl");
	int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	cell_update_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Load the Join Compute Shader */
	shader_code = LoadFileText("Shaders/collision_data_join.glsl");
	shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	join_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Load the Narrow Phase Compute Shader */
	shader_code = LoadFileText("Shaders/gpu_narrow.glsl");
	shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	narrow_phase_shader_id = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	/* Create a GPUVector for the active pairs */
	active_entities = std::make_unique<GPUVector<ActiveEntity>>();
	active_entities->resize(10000, { 0, });

	/* Create a GPUVector for the program parameters */
	program_params = std::make_unique<GPUVector<ProgramParams>>();
	program_params->resize(1, { static_cast<uint64_t>(wanted_comp_sig.to_ulong()), 0 , 0, 0 });

	/* Create a GPUVector for the added entities every frame */
	added_entities = std::make_unique<GPUVector<Entity>>();
	added_entities->resize(100, { Entity() });

	/* Create a GPUVector for the deleted entities every frame */
	deleted_entities = std::make_unique<GPUVector<Entity>>();
	deleted_entities->resize(100, { Entity() });

	/* Create a GPUVector for the colliding entities every frame */
	colliding_entities = std::make_unique<GPUVector<CollisionPair>>();
	colliding_entities->resize(max_colliding_entities, { CollisionPair()});

	/* Create a GPUVector for the deleted entities every frame */
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
	ClearGrid();
	Join();
	UpdateCells();
	GPUNarrowPhase();
	CheckCollisions();
}

void CollisionSystem::Join()
{
	/* The Join Shader only needs to do work if Tranform or Spatial Info Components were added/deleted */
	if (entities_added_this_frame == 0 && entities_deleted_this_frame == 0)
	{
		return;
	}

	glUseProgram(join_shader_id);

	/* Get the needed buffers from the current Registry */
	uint32_t signatureBuffer = Application::GetCurrentRegistry().signatures.GetSSBOID();
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Collider>& spatial_info_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Collider>();

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

	/* Bind the Spatial Info Sparse Array to slot 4 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, spatial_info_array.GetSparseSSBO());

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


uint32_t Mupfel::CollisionSystem::WorldtoCell(Coordinate<uint32_t> c)
{
	uint32_t cell_x = c.x >> collision_grid.cell_size_pow;
	uint32_t cell_y = c.y >> collision_grid.cell_size_pow;

	cell_x = std::min(cell_x, collision_grid.num_cells_x - 1);
	cell_y = std::min(cell_y, collision_grid.num_cells_y - 1);

	return cell_y * collision_grid.num_cells_x + cell_x;
}

uint32_t Mupfel::CollisionSystem::PointXtoCell(uint32_t x)
{
	uint32_t cell_x = x >> collision_grid.cell_size_pow;
	return std::min(cell_x, collision_grid.num_cells_x - 1);
}

uint32_t Mupfel::CollisionSystem::PointYtoCell(uint32_t y)
{
	uint32_t cell_y = y >> collision_grid.cell_size_pow;
	return std::min(cell_y, collision_grid.num_cells_y - 1);
}

void Mupfel::CollisionSystem::SetProgramParams()
{
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();

	/* Update the Shader Program parameters for the GPU */
	ProgramParams params{};

	params.component_mask = static_cast<uint64_t>(wanted_comp_sig.to_ulong());
	params.entities_added = entities_added_this_frame;
	params.entities_deleted = entities_deleted_this_frame;
	params.active_entities = transform_array.Size();
	params.cell_size_pow = 6;
	params.num_cells_x = 64;
	params.num_cells_y = 64;
	params.entities_per_cell = 2048;
	params.max_colliding_entities = max_colliding_entities;

	glNamedBufferSubData(programParamsSSBO, 0, sizeof(ProgramParams), &params);
}

void Mupfel::CollisionSystem::UpdateCells()
{
	glUseProgram(cell_update_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Collider>& spatial_info_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Collider>();

	/* Bind the Collision Grid Cell Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, collision_grid.cells.GetSSBOID());

	/* Bind the Collision Grid Entity Array to slot 2 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, collision_grid.entities.GetSSBOID());

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

	/* Bind the Spatial Info Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spatial_info_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetSSBOID());

	/* Bind the Shader parameters to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	GLuint groups = (active_entities->size() + 255) / 256;
	glDispatchCompute(groups, 1, 1);
	glFinish();
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

void Mupfel::CollisionSystem::GPUNarrowPhase()
{
	glUseProgram(narrow_phase_shader_id);

	/* Get the needed buffers from the current Registry */
	GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
	GPUComponentArray<Mupfel::Collider>& spatial_info_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Collider>();

	/* Bind the Collision Grid Cell Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, collision_grid.cells.GetSSBOID());

	/* Bind the Collision Grid Entity Array to slot 2 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, collision_grid.entities.GetSSBOID());

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetSparseSSBO());

	/* Bind the Transform Component Array to slot 4 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, transform_array.GetComponentSSBO());

	/* Bind the Spatial Info Sparse Array to slot 5 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, spatial_info_array.GetSparseSSBO());

	/* Bind the Spatial Info Component Array to slot 6 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spatial_info_array.GetComponentSSBO());

	/* Bind the Active Pairs Array to slot 7 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetSSBOID());

	/* Bind the Shader parameters to slot 8 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, programParamsSSBO);

	/* Bind the Colliding Entities Array to slot 9 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, colliding_entities->GetSSBOID());

	/* Bind buffer for the number of colliding entities to slot 10 */
	num_colliding_entities->operator[](0) = 0;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, num_colliding_entities->GetSSBOID());

	GLuint groups = ((64*64) + 255) / 256;
	glDispatchCompute(groups, 1, 1);
	glFinish();
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
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

#if 0
			Transform t_a = registry.GetComponent<Transform>(a);
			Transform t_b = registry.GetComponent<Transform>(b);

			Mupfel::Rectangle::RaylibDrawRectFilled(t_a.pos_x - col_a.GetBoundingBox() / 2.0f, t_a.pos_y - col_a.GetBoundingBox() / 2.0f, col_a.GetBoundingBox(), col_a.GetBoundingBox(), 234, 72, 0, 255);
			Mupfel::Rectangle::RaylibDrawRectFilled(t_b.pos_x - col_b.GetBoundingBox() / 2.0f, t_b.pos_y - col_b.GetBoundingBox() / 2.0f, col_b.GetBoundingBox(), col_b.GetBoundingBox(), 234, 72, 0, 255);
#endif
		}

		// Draw them
		
	}
}

void Mupfel::CollisionSystem::ClearGrid()
{
	for (uint32_t y = 0; y < collision_grid.num_cells_y; y++)
	{
		for (uint32_t x = 0; x < collision_grid.num_cells_x; x++)
		{
			/* Calculate the index of the wanted cell in the cell array */
			uint32_t cell_index = y * collision_grid.num_cells_x + x;
			if (collision_grid.cells[cell_index].count == 0)
			{
				continue;
			}

			uint32_t start_index = collision_grid.cells[cell_index].startIndex;

			for (uint32_t i = 0; i < collision_grid.cells[cell_index].count; i++)
			{
				collision_grid.entities[start_index + i] = Entity();
			}
			collision_grid.cells[cell_index].count = 0;
		}
	}

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

void Mupfel::CollisionSystem::SetCallbacks()
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

			if (entities_deleted_this_frame >= deleted_entities->size())
			{
				deleted_entities->resize(entities_deleted_this_frame * 2, Entity());
			}

			deleted_entities->operator[](entities_deleted_this_frame) = event.e;

			entities_deleted_this_frame++;
		}
	);
}
