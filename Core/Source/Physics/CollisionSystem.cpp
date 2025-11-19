#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>
#include <array>
#include <thread>
#include "Core/Application.h"

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

/* Raylib/OpenGL */
#include "glad.h"
#include "raylib.h"
#include "rlgl.h"

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
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Collider>();

/**
 * @brief OpenGL shader program ID for the main movement update compute shader.
 */
static uint32_t cell_update_shader_id = 0;

/**
 * @brief OpenGL shader program ID for the data join compute shader, used to sync active entity lists.
 */
static uint32_t join_shader_id = 0;

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
	CheckInteractions();
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

	/* Bind the Collision Grid Entity Array to slot 1 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, collision_grid.entities.GetSSBOID());

	/* Bind the Transform Component Array to slot 3 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

	/* Bind the Spatial Info Sparse Array to slot 4 */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, spatial_info_array.GetSparseSSBO());

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

void Mupfel::CollisionSystem::CheckInteractions()
{
	static uint32_t num_rows = collision_grid.num_cells_y;
	static uint32_t num_columns = collision_grid.num_cells_x;
	static uint32_t cell_size = 1 << collision_grid.cell_size_pow;

	uint32_t pos_x = 0;
	uint32_t pos_y = 0;

	for (uint32_t y = 0; y < num_rows; y++)
	{
		for (uint32_t x = 0; x < num_columns; x++)
		{
			uint32_t cell_index = CollisionSystem::WorldtoCell({ pos_x, pos_y });
			uint32_t cell_count = collision_grid.cells[cell_index].count;
		
			if (cell_count > 1)
			{
				/* Check collision */
				uint32_t cell_start_index = collision_grid.cells[cell_index].startIndex;
				CheckEntityInteraction(cell_start_index, cell_count);
			}

			pos_x += cell_size;
		}
		pos_x = 0;
		pos_y += cell_size;
	}
}

void Mupfel::CollisionSystem::CheckEntityInteraction(uint32_t start, uint32_t num_ents)
{
	//TraceLog(LOG_INFO, "Checking collision between %u entities", num_ents);

	for (uint32_t first = 0; first < num_ents; first++)
	{
		for (uint32_t second = first + 1; second < num_ents; second++)
		{
			Entity first_entity = collision_grid.entities[start + first];
			Entity second_entity = collision_grid.entities[start + second];
			Collider first_collider = registry.GetComponent<Collider>(first_entity);
			Collider second_collider = registry.GetComponent<Collider>(second_entity);
			Transform first_transform = registry.GetComponent<Transform>(first_entity);
			Transform second_transform = registry.GetComponent<Transform>(second_entity);

			
		}
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
