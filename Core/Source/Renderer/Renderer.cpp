#include "Renderer.h"
#include "raylib.h"
#include "glad.h"

#include "rlgl.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/View.h"
#include <vector>

using namespace Mupfel;

/* For now, use a default Triangle location (in 2D space) */
static float vertices[] = {
     0.5f,  0.5f, 0.0f, // Top-Right
     0.5f, -0.5f, 0.0f, // Bottom-Right
    -0.5f, -0.5f, 0.0f, // Bottom-Left
    -0.5f,  0.5f, 0.0f  // Top-Left
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};

static unsigned int vertex_buffer;
static unsigned int element_buffer_object;
static unsigned int vertex_array_object;
static unsigned int vertex_shader;
static unsigned int fragment_shader;
static unsigned int shader_program;

void Renderer::Init()
{
	/* Generate a new Vertex Buffer */
    glGenBuffers(1, &vertex_buffer);
    glGenBuffers(1, &element_buffer_object);

    /* Gen a corresbonding Vertex Array Object */
    glGenVertexArrays(1, &vertex_array_object);

    /* Configure the Vertex Array */
    glBindVertexArray(vertex_array_object);

    /* Bind our Vertex Buffer to GL_ARRAY_BUFFER (which is the vertex buffer type) */
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    /*
        copy our triangle verticies into the vertex buffer. Currently we use GL_STATIC_DRAW,
        because the triangle position does not change. If we update the verticies in the future,
        we should use GL_DYNAMIC_DRAW.
    */
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* Bind the EBO buffer */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    /* Tell OpenGL how to interpret our vertex data */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));

    glEnableVertexAttribArray(0);

    /* Load Our Shaders */
    ImportVertexShader();
    ImportFragmentShader();

    /* Create the Shader Program */
    CreateShaderProgram();

    /* Use the created Shader Program */
    glUseProgram(shader_program);

    /* Shaders are no longer needed, delete them */
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
}

void Renderer::Render()
{

    if (!IsWindowReady()) {
        TraceLog(LOG_ERROR, "Renderer: window context not ready!");
        return;
    }

    glUseProgram(shader_program);
    glBindVertexArray(vertex_array_object);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::DeInit()
{
	
}

void Mupfel::Renderer::ImportVertexShader()
{
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
}

void Mupfel::Renderer::ImportFragmentShader()
{
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
}

void Mupfel::Renderer::CreateShaderProgram()
{
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

}
