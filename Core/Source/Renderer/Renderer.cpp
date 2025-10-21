#include "Renderer.h"
#include "raylib.h"
#include "glad.h"
#include "rlgl.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/View.h"
#include <vector>

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "raygui.h"

#include "Core/Profiler.h"
#include "Texture.h"

using namespace Mupfel;

static Shader shader;
static unsigned int VAO;
static unsigned int VBO;
static unsigned int EBO;



static float vertices_def[] = {
     -32.0f, -32.0f, 0.0f, 0.0f, 0.0f, // bottom-left
      32.0f, -32.0f, 0.0f, 1.0f, 0.0f, // bottom-right 
      32.0f,  32.0f, 0.0f, 1.0f, 1.0f, // top-right
      -32.0f, 32.0f, 0.0f, 0.0f, 1.0f, //top-left
};

static glm::vec3 cubePositions[] = {
    glm::vec3(100.0f,  100.0f,  0.0f),
    glm::vec3(200.0f,  200.0f, 0.0f),
    glm::vec3(300.0f,  300.0f, 0.0f),
    glm::vec3(400.0f,  400.0f, 0.0f),
    glm::vec3(500.0f,  500.0f, 0.0f),
    glm::vec3(600.0f,  600.0f, 0.0f),
    glm::vec3(700.0f,  700.0f, 0.0f),
    glm::vec3(800.0f,  800.0f, 0.0f),
    glm::vec3(900.0f,  900.0f, 0.0f),
    glm::vec3(1000.0f, 1000.0f, 0.0f)
};

unsigned short indices[] = {
    0, 1, 2,
    0, 2, 3
};

static Mupfel::Texture *t;

static bool renderModeCustom = true;


void Renderer::Init()
{
    shader = LoadShader("Shaders/simple_vertex_shader.glsl", "Shaders/simple_fragment_shader.glsl");

    VAO = rlLoadVertexArray();
    rlEnableVertexArray(VAO);

    VBO = rlLoadVertexBuffer(vertices_def, sizeof(vertices_def), false);
    EBO = rlLoadVertexBufferElement(indices, sizeof(indices), false);

    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 5 * sizeof(float),0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, 0, 5 * sizeof(float), sizeof(float) * 3);
    rlEnableVertexAttribute(0);
    rlEnableVertexAttribute(1);

    t = new Texture("Resources/simple_ball.png");

}

void Renderer::Render()
{
    ProfilingSample prof("Renderer::Update()");

    if (!IsWindowReady()) {
        TraceLog(LOG_ERROR, "Renderer: window context not ready!");
        return;
    }

    if (!renderModeCustom) {
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
        int screen_width = Application::GetCurrentRenderWidth();
        int screen_height = Application::GetCurrentRenderHeight();

        auto entity_view = Application::GetCurrentRegistry().view<Transform>();

        std::vector<glm::vec3> position_vector;

        for (auto [entity, transform] : entity_view)
        {
            position_vector.push_back(glm::vec3(transform.pos.x, transform.pos.y, 0.0f));
        }

        glm::mat4 view = glm::mat4(1.0f);

        glm::mat4 projection = glm::mat4(1.0f);

        projection = glm::ortho(0.0f, static_cast<float>(screen_width), 0.0f, static_cast<float>(screen_height), -1.0f, 1.0f);


        rlEnableShader(shader.id);

        rlEnableTexture(t->id);

        int modelLoc = glGetUniformLocation(shader.id, "model");
        if (modelLoc == -1)
        {
            TraceLog(LOG_ERROR, "Could not find uniform");
        }


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

        rlEnableVertexArray(VAO);

        for (auto& pos : position_vector)
        {
            glm::mat4 model = glm::mat4(1.0f);
            //model = glm::rotate(model, glm::radians(rotation_val), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::translate(model, pos);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            rlDrawVertexArrayElements(0, 6, 0);
        }


        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            std::cout << "OpenGL Error: " << err << std::endl;
        }

        rlDisableVertexArray();

        rlDisableShader();
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
