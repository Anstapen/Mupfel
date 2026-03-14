#include "Renderer.h"
#include "raylib.h"
#include "glad.h"
#include "rlgl.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
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
    uint64_t active_entities = 0;
};


static const unsigned short QUAD_IDX[] = { 0,2,1, 0,3,2 };

static Mupfel::Texture *t;

static glm::mat4 view = glm::mat4(1.0f);

static glm::mat4 projection = glm::mat4(1.0f);

static int screen_w = 0;
static int screen_h = 0;

static uint32_t join_compute_shader = 0;
static uint32_t prepare_render_shader = 0;

static std::unique_ptr<GPUComponentArray<uint32_t>> active_entities = nullptr;

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

    /* Load the Prepare Render Compute Shader */
    char* shader_code = LoadFileText("Shaders/prepare_render_pass.glsl");
    int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
    prepare_render_shader = rlLoadComputeShaderProgram(shader_data);
    UnloadFileText(shader_code);

    /* Create a GPUVector for the active entities */
    active_entities = std::make_unique<GPUComponentArray<uint32_t>>();

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
            active_entities->Remove(event.e);
        }
    );
}

void Renderer::Render()
{
    ProfilingSample prof("Renderer custom Draw Batching");

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

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, transform_array.GetSparseSSBO());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, texture_array.GetSparseSSBO());

    /* Bind the Texture Component Array to slot 6 */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, texture_array.GetComponentSSBO());

    /* Bind the Active Pairs Array to slot 7 */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, active_entities->GetComponentSSBO());

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
    }
    
}

void Mupfel::Renderer::SetProgramParams()
{
    /* Update the Shader Program parameters for the GPU */
    ProgramParams params{};

    params.active_entities = active_entities->Size();

    glNamedBufferSubData(frameParamsSSBO, 0, sizeof(ProgramParams), &params);
}
