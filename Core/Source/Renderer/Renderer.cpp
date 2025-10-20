#include "Renderer.h"
#include "raylib.h"
#include "glad.h"
#include "rlgl.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/View.h"
#include <vector>

using namespace Mupfel;

static Shader shader;
static unsigned int VAO;
static unsigned int VBO;
static unsigned int EBO;

/* For now, use a default Triangle location (in 2D space) */
static float vertices_ebo[] = {
     0.5f,  0.5f, 0.0f, // Top-Right
     0.5f, -0.5f, 0.0f, // Bottom-Right
    -0.5f, -0.5f, 0.0f, // Bottom-Left
    -0.5f,  0.5f, 0.0f  // Top-Left
};

static float vertices_def[] = {
     -0.5f, -0.5f, 0.0f, // left  
      0.5f, -0.5f, 0.0f, // right 
      0.0f,  0.5f, 0.0f  // top 
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};



void Renderer::Init()
{

    shader = LoadShader("Shaders/simple_vertex_shader.glsl", "Shaders/simple_fragment_shader.glsl");

    VAO = rlLoadVertexArray();
    rlEnableVertexArray(VAO);

    VBO = rlLoadVertexBuffer(vertices_def, sizeof(vertices_def), false);
    EBO = rlLoadVertexBufferElement(indices, sizeof(indices), false);

    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 3 * sizeof(float), 0);
    rlEnableVertexAttribute(0);

    
}

void Renderer::Render()
{

    if (!IsWindowReady()) {
        TraceLog(LOG_ERROR, "Renderer: window context not ready!");
        return;
    }

    rlEnableShader(shader.id);

    rlEnableVertexArray(VAO);
    rlDrawVertexArray(0, 3);
    //rlDrawVertexArrayElements(0, 6, 0);
    rlDisableVertexArray();

    rlDisableShader();
    
}

void Renderer::DeInit()
{
	
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
