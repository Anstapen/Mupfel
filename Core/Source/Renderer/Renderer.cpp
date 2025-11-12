#include "Renderer.h"
#include "raylib.h"
#include "glad.h"
#include "rlgl.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/View.h"
#include "ECS/Registry.h"
#include <vector>
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "raygui.h"
#include "Core/Profiler.h"
#include "Texture.h"
#include <algorithm>
#include "Physics/MovementSystem.h"

using namespace Mupfel;

static Shader shader;

static unsigned int VAO = 0;
static unsigned int quadVBO = 0;
static unsigned int EBO = 0;
static unsigned int SSBO = 0;

// static Unit-Quad (centered) with pos+uv
static const float QUAD_VERTS[] = {
    //  x      y      z     u     v
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,  // 0
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,  // 1
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f,  // 2
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f   // 3
};

// entspricht GL's DrawElementsIndirectCommand
struct DrawElementsIndirectCommand {
    uint32_t count;         // number of indices per instance (bei dir: 6)
    uint32_t instanceCount; // wird vom Compute gesetzt (== aktive Entities)
    uint32_t firstIndex;    // meist 0
    uint32_t baseVertex;    // meist 0
    uint32_t baseInstance;  // meist 0
};

struct ProgramParams {
    uint64_t component_mask = 0;
    uint64_t active_entities = 0;
    uint64_t entities_added = 0;
    uint64_t entities_deleted = 0;
    float delta_time;
};


static const unsigned short QUAD_IDX[] = { 0,2,1, 0,3,2 };

static Mupfel::Texture *t;

static glm::mat4 view = glm::mat4(1.0f);

static glm::mat4 projection = glm::mat4(1.0f);

static int screen_w = 0;
static int screen_h = 0;

static uint32_t join_compute_shader = 0;
static uint32_t prepare_render_shader = 0;

struct ActiveEntity {
    uint32_t entity_id = 0;
    uint32_t transform_index = 0;
    uint32_t texture_index = 0;
};

static std::unique_ptr<GPUVector<ActiveEntity>> active_entities = nullptr;
static std::unique_ptr<GPUVector<uint32_t>> active_entity_count = nullptr;

static std::unique_ptr<GPUVector<Entity>> added_entities = nullptr;
static uint32_t entities_added_this_frame = 0;

static std::unique_ptr<GPUVector<Entity>> deleted_entities = nullptr;
static uint32_t entities_deleted_this_frame = 0;

GLuint indirectBuffer = 0;
static GLuint frameParamsSSBO = 0;
GLuint texturesSSBO = 0;
const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::TextureComponent>();


void Renderer::Init()
{
    shader = LoadShader("Shaders/simple_vertex_shader.glsl", "Shaders/simple_fragment_shader.glsl");

    VAO = rlLoadVertexArray();
    rlEnableVertexArray(VAO);

    /* statischer VBO */
    quadVBO = rlLoadVertexBuffer(QUAD_VERTS, sizeof(QUAD_VERTS), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 5 * sizeof(float), 0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, 0, 5 * sizeof(float), sizeof(float) * 3);
    rlEnableVertexAttribute(0);
    rlEnableVertexAttribute(1);

    EBO = rlLoadVertexBufferElement(QUAD_IDX, sizeof(QUAD_IDX), false);

    /* Load the texture */
    t = new Texture("Resources/simple_ball.png");

    /* Load the Join Compute Shader */
    char* shader_code = LoadFileText("Shaders/render_data_join.glsl");
    int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
    join_compute_shader = rlLoadComputeShaderProgram(shader_data);
    UnloadFileText(shader_code);

    /* Load the Prepare Render Compute Shader */
    shader_code = LoadFileText("Shaders/prepare_render_pass.glsl");
    shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
    prepare_render_shader = rlLoadComputeShaderProgram(shader_data);
    UnloadFileText(shader_code);

    /* Create a GPUVector for the active entities */
    active_entities = std::make_unique<GPUVector<ActiveEntity>>();
    active_entities->resize(1000, { 0, 0, 0 });

    /* Create a GPUVector for the added entities every frame */
    added_entities = std::make_unique<GPUVector<Entity>>();
    added_entities->resize(100, { Entity() });

    /* Create a GPUVector for the deleted entities every frame */
    deleted_entities = std::make_unique<GPUVector<Entity>>();
    deleted_entities->resize(100, { Entity() });

    glCreateBuffers(1, &indirectBuffer);
    DrawElementsIndirectCommand cmd{};
    cmd.count = 6;
    cmd.instanceCount = 0;
    cmd.firstIndex = 0;
    cmd.baseVertex = 0;
    cmd.baseInstance = 0;

    // Speicher anlegen; Flags wie bei deinen SSBOs (Storage + optional Mapping)
    glNamedBufferStorage(indirectBuffer, sizeof(cmd), &cmd,
        GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    glCreateBuffers(1, &frameParamsSSBO);
    glNamedBufferStorage(frameParamsSSBO, sizeof(ProgramParams), nullptr, GL_DYNAMIC_STORAGE_BIT);


    rlEnableShader(shader.id);

    int viewlLoc = glGetUniformLocation(shader.id, "view");
    if (viewlLoc == -1)
    {
        TraceLog(LOG_ERROR, "Could not find uniform");
    }
    glUniformMatrix4fv(viewlLoc, 1, GL_FALSE, glm::value_ptr(view));

    int projLoc = glGetUniformLocation(shader.id, "projection");
    if (projLoc == -1)
    {
        TraceLog(LOG_ERROR, "Could not find uniform");
    }
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLuint64 handle = glGetTextureHandleARB(t->id);
    glMakeTextureHandleResidentARB(handle);
    
    glCreateBuffers(1, &texturesSSBO);
    glNamedBufferStorage(texturesSSBO, sizeof(GLuint64), &handle, GL_DYNAMIC_STORAGE_BIT);

    rlDisableShader();

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
                /* Entity does not have Transform + Texture */
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

            Entity::Signature texture_sig;
            texture_sig.set(ComponentIndex::Index<Mupfel::TextureComponent>());

            uint32_t has_transform_component = (event.sig & transform_sig) != 0 ? 1 : 0;
            uint32_t has_texture_component = (event.sig & texture_sig) != 0 ? 1 : 0;

            uint32_t comp_info = has_transform_component + has_texture_component;

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

void Renderer::Render()
{
    ProfilingSample prof("Renderer custom Draw Batching");

    auto view = Application::GetCurrentRegistry().view<Transform>();

    SetProgramParams();

    JoinAndRender();
    
}

void Renderer::DeInit()
{
	
}

void Mupfel::Renderer::UpdateScreenSize()
{
    int current_screen_w = Application::GetCurrentRenderWidth();
    int current_screen_h = Application::GetCurrentRenderHeight();

    bool screen_changed = false;

    if (current_screen_h != screen_h)
    {
        screen_h = current_screen_h;
        screen_changed = true;
    }

    if (current_screen_w != screen_w)
    {
        screen_w = current_screen_w;
        screen_changed = true;
    }

    if (screen_changed)
    {
        /* Update perspective matrix */
        projection = glm::ortho(0.0f, (float)screen_w, (float)screen_h, 0.0f, -1.0f, 1.0f);

        rlEnableShader(shader.id);
        int projLoc = glGetUniformLocation(shader.id, "projection");
        if (projLoc == -1)
        {
            TraceLog(LOG_ERROR, "Could not find uniform");
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        rlDisableShader();
    }
    

}

void Mupfel::Renderer::JoinAndRender()
{
    GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();
    GPUComponentArray<TextureComponent>& texture_array = Application::GetCurrentRegistry().GetComponentArray<TextureComponent>();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, frameParamsSSBO);

    /* Bind the Transform Component Array to slot 3 */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, transform_array.GetComponentSSBO());

    /* Bind the Texture Component Array to slot 6 */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, texture_array.GetComponentSSBO());

    /* Bind the Active Pairs Array to slot 7 */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetSSBOID());

    {
        ProfilingSample prof("Running Render Join");

        if (entities_added_this_frame > 0 || entities_deleted_this_frame > 0)
        {
            glUseProgram(join_compute_shader);

            uint32_t signatureBuffer = Application::GetCurrentRegistry().signatures.GetSSBOID();

            /*
                Check if we need to resize the Active Entity buffer.
                As an active entity needs to have both transform and veloctiy,
                the min of both arrays is the maximum number of active entities this frame.
            */
            uint32_t max_active_pairs = std::min<uint32_t>(transform_array.Size(), texture_array.Size());

            if (transform_array.Size() >= active_entities->size())
            {
                active_entities->resize(transform_array.Size() * 2, { 0, 0, 0 });
            }

            /* Bind the Entity Signature Array to slot 0 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, signatureBuffer);

            /* Bind the Transform Sparse Array to slot 1 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, transform_array.GetSparseSSBO());

            /* Bind the Transform Dense Array to slot 2 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, transform_array.GetDenseSSBO());

            /* Bind the Texture Sparse Array to slot 4 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, texture_array.GetSparseSSBO());

            /* Bind the Texture Dense Array to slot 5 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, texture_array.GetDenseSSBO());

            /* Bind the Added Entities Array to slot 10 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, added_entities->GetSSBOID());

            /* Bind the Deleted Entities Array to slot 11 */
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, deleted_entities->GetSSBOID());

            uint32_t changed_entities = std::max<uint32_t>(entities_added_this_frame, entities_deleted_this_frame);

            //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            // Compute starten
            GLuint groups = (changed_entities + 255) / 256;
            glDispatchCompute(groups, 1, 1);
            glFinish();

            entities_added_this_frame = 0;
            entities_deleted_this_frame = 0;
            //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        }
    }

    {
        glUseProgram(prepare_render_shader);

        // binde den Indirect-Buffer **zusätzlich** als SSBO (binding=9)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, indirectBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, frameParamsSSBO);

        glDispatchCompute(1, 1, 1);

        // sehr wichtig: Indirect-Command Updates sind Befehlsdaten : Command-Barrier!
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    }

    {
        ProfilingSample prof("Running Graphics Pipeline");

        UpdateScreenSize();

        rlEnableShader(shader.id);
       
        /* Update the Vertex Array Buffer and index buffer */
        rlEnableVertexArray(VAO);
        rlEnableVertexBuffer(quadVBO);
        rlEnableVertexBufferElement(EBO);
        
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, texturesSSBO);


        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0);

        glFinish();
        // und zeichne GPU-driven:
        
        
        // optional finale Barrier, wenn danach SSBO/VS/DMA liest
        
        //rlDrawVertexArrayElementsInstanced(0, 6, 0, active_entities);

        //rlDisableVertexArray();
        //rlDisableShader();
    }
    
}

void Mupfel::Renderer::SetProgramParams()
{
    GPUComponentArray<Mupfel::Transform>& transform_array = Application::GetCurrentRegistry().GetComponentArray<Mupfel::Transform>();

    /* Update the Shader Program parameters for the GPU */
    ProgramParams params{};

    params.component_mask = static_cast<uint64_t>(wanted_comp_sig.to_ulong());
    params.entities_added = entities_added_this_frame;
    params.entities_deleted = entities_deleted_this_frame;
    params.active_entities = transform_array.Size();
    params.delta_time = 0;

    glNamedBufferSubData(frameParamsSSBO, 0, sizeof(ProgramParams), &params);
}
