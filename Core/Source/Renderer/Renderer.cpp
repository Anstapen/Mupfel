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

using namespace Mupfel;

static Shader shader;

static unsigned int VAO = 0;
static unsigned int instanceVBO = 0;
static unsigned int quadVBO = 0;
static unsigned int EBO = 0;

struct InstanceData
{
    // Wir schicken eine Model-Matrix als mat4 (64 Bytes)
    // plus Farbe (16B) + TexIndex (4B als float) -> 84B pro Instance.
    //glm::mat4 model;
    glm::vec2 pos;
    glm::vec2 velocity;
    glm::vec2 scale;
    glm::vec2 rotation;
    //glm::vec4 color;     // z.B. (1,1,1,1)
    //float texIndex;      // als float; im Shader wieder als float
    //float _pad[3];       // Padding auf 16-Byte (optional, für Ordnung)
};

static_assert(sizeof(InstanceData) == 32);

// Statisches Unit-Quad (centered) mit pos+uv
static const float QUAD_VERTS[] = {
    //  x      y      z     u     v
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,  // 0
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,  // 1
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f,  // 2
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f   // 3
};

static const unsigned short QUAD_IDX[] = { 0,2,1, 0,3,2 };


static std::vector<InstanceData> instances;

static Mupfel::Texture *t;

static bool renderModeCustom = true;
static size_t instanceVBOSize = 0;
static void* mappedPtr = nullptr;

static constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

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

    /* instanced VBO */
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    size_t initialCap = 10000;
    instances.reserve(initialCap);
    
    AllocateInstanceBuffer(initialCap * sizeof(InstanceData));

    /* Load Copute Shader + Buffers */


    t = new Texture("Resources/simple_ball.png");

}

void Renderer::Render()
{
    

    if (!IsWindowReady()) {
        TraceLog(LOG_ERROR, "Renderer: window context not ready!");
        return;
    }

    if (!renderModeCustom) {
        ProfilingSample prof("Renderer Raylib DrawTexture calls");
        /* Render all entities */
        auto view = Application::GetCurrentRegistry().view<Transform>();

        for (auto [entity, transform] : view)
        {
            uint32_t render_pos_x = transform.pos.x - (t->width / 2);
            uint32_t render_pos_y = transform.pos.y - (t->height / 2);
            Texture::RaylibDrawTexture(*t, render_pos_x, render_pos_y);
        }
    }
    else {
        ProfilingSample prof("Renderer custom Draw Batching");

        auto entity_view = Application::GetCurrentRegistry().view<TextureComponent, Transform>();

        UpdateScreenSize();

        {
            ProfilingSample prof("Filling Instance vector ");
            instances.clear();

            for (auto [entity, texture, transform] : entity_view)
            {
                float w = float(texture.texture->width);
                float h = float(texture.texture->height);

                InstanceData inst{};
                inst.pos.x = transform.pos.x;
                inst.pos.y = transform.pos.y;
                inst.scale.x = transform.scale_x * w;
                inst.scale.y = transform.scale_y * h;
                inst.rotation.x = transform.rotation;
                instances.push_back(inst);
            }
        }

        const GLsizei instanceCount = static_cast<GLsizei>(instances.size());
        if (instanceCount == 0) return;


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
        
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        const GLsizeiptr bytes = instanceCount * sizeof(InstanceData);

        if (bytes > instanceVBOSize)
        {
            TraceLog(LOG_WARNING, "Instance buffer overflow! Increasing buffer.");
            TraceLog(LOG_WARNING, "Old size: %u, New Size: %u", instanceVBOSize, bytes * 2);
            size_t newSize = std::max<size_t>(bytes, instanceVBOSize * 2);
            AllocateInstanceBuffer(bytes * 2);
        }
        else
        {
            // Direkt in den gemappten GPU-Speicher schreiben
            std::memcpy(mappedPtr, instances.data(), bytes);
        }

        //glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, instances.data());

        rlEnableVertexBuffer(quadVBO);
        rlEnableVertexBufferElement(EBO);
        rlDrawVertexArrayElementsInstanced(0, 6, 0, instanceCount);

        //rlDisableVertexArray();
        //rlDisableShader();
    }
    
}

void Renderer::DeInit()
{
	
}

void Mupfel::Renderer::ToggleMode()
{
    renderModeCustom = !renderModeCustom;
    TraceLog(LOG_INFO, "Custom Render Mode: %s", renderModeCustom ? "yes" : "no");
}

void Mupfel::Renderer::ImportVertexShader()
{
#if 0
    char* vertex_shader_source = LoadFileText("Shaders/simple_vertex_shader.glsl");
    if (vertex_shader_source == nullptr)
    {
        TraceLog(LOG_ERROR, "Could not load Vertex Shader!");
    }

    /* Compile the given Vertex Shader */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);

    /* Check if compilation was successful */
    int success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
        TraceLog(LOG_ERROR, "Could not compile the Vertex Shader:");
        TraceLog(LOG_ERROR, infoLog);
    }
    /* Unload the text file */
    UnloadFileText(vertex_shader_source);
#endif
}

void Mupfel::Renderer::ImportFragmentShader()
{
#if 0
    char* fragment_shader_source = LoadFileText("Shaders/simple_fragment_shader.glsl");
    if (fragment_shader_source == nullptr)
    {
        TraceLog(LOG_ERROR, "Could not load Fragment Shader!");
    }



    /* Compile the Fragment Shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);

    /* Check if compilation was successful */
    int success;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragment_shader, 512, nullptr, infoLog);
        TraceLog(LOG_ERROR, "Could not compile the Fragment Shader:");
        TraceLog(LOG_ERROR, infoLog);
    }

    /* Unload the text file */
    UnloadFileText(fragment_shader_source);
#endif
}

void Mupfel::Renderer::CreateShaderProgram()
{
#if 0
    /* Create the shader program */
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    /* Check if linking was successful */
    int success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
        TraceLog(LOG_ERROR, "Could not link the Shader Program:");
        TraceLog(LOG_ERROR, infoLog);
    }
#endif
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

void Mupfel::Renderer::AllocateInstanceBuffer(size_t newCapacity)
{
    if (instanceVBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
        mappedPtr = nullptr;
    }

    // Neuen Buffer erstellen
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferStorage(GL_ARRAY_BUFFER, newCapacity, nullptr, flags);

    mappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, newCapacity, flags);
    if (!mappedPtr)
    {
        TraceLog(LOG_ERROR, "Persistent mapping failed (realloc)!");
    }

    instanceVBOSize = newCapacity;

    /* Attribute layout for the model matrix */
    size_t stride = sizeof(InstanceData);

    // layout (location = 2): vec2 iPos
    rlSetVertexAttribute(2, 2, RL_FLOAT, false, stride, offsetof(InstanceData, pos));
    rlEnableVertexAttribute(2);
    rlSetVertexAttributeDivisor(2, 1);

    // layout (location = 3): vec2 iScale
    rlSetVertexAttribute(3, 2, RL_FLOAT, false, stride, offsetof(InstanceData, scale));
    rlEnableVertexAttribute(3);
    rlSetVertexAttributeDivisor(3, 1);

    // layout (location = 4): float iRotation
    rlSetVertexAttribute(4, 2, RL_FLOAT, false, stride, offsetof(InstanceData, rotation));
    rlEnableVertexAttribute(4);
    rlSetVertexAttributeDivisor(4, 1);
}
