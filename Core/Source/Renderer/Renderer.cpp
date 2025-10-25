#include "Renderer.h"
#include "raylib.h"
#include "glad.h"
#include "rlgl.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/View.h"
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

static uint32_t ent_count = 0;

// static Unit-Quad (centered) with pos+uv
static const float QUAD_VERTS[] = {
    //  x      y      z     u     v
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,  // 0
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,  // 1
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f,  // 2
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f   // 3
};

static const unsigned short QUAD_IDX[] = { 0,2,1, 0,3,2 };

static Mupfel::Texture *t;

static glm::mat4 view = glm::mat4(1.0f);

static glm::mat4 projection = glm::mat4(1.0f);

static int screen_w = 0;
static int screen_h = 0;


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

    /* Register a listener to get updated */
    Application::GetCurrentEventSystem().RegisterListener<MovementSystemUpdateEvent>(
        [](const MovementSystemUpdateEvent& evt)
        {
            SSBO = evt.ssbo_id;
            ent_count = evt.ssbo_size;
        }
    );

}

void Renderer::Render()
{
    ProfilingSample prof("Renderer custom Draw Batching");

    auto entity_view = Application::GetCurrentRegistry().view<TextureComponent, Transform>();

    UpdateScreenSize();

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

    // Textur binden:
    // A) Einzel-Textur:
    rlActiveTextureSlot(0);
    rlEnableTexture(t->id);
    int uTex = glGetUniformLocation(shader.id, "uTex");
    if (uTex >= 0) glUniform1i(uTex, 0);

    /* Update the Vertex Array Buffer and index buffer */
    rlEnableVertexArray(VAO);

    rlEnableVertexBuffer(quadVBO);
    rlEnableVertexBufferElement(EBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
    rlDrawVertexArrayElementsInstanced(0, 6, 0, ent_count);

    //rlDisableVertexArray();
    //rlDisableShader();
    
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
    }

}
