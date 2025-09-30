#include "Engine.h"
#include "raylib.h"
#include <iostream>


using namespace Mupfel;

Engine::Engine() : event_system(), input_manager(event_system, InputManager::Mode::MOUSE_KEYBOARD)
{
}

Engine::~Engine()
{
}

static float start_time = 0.0f;
static float current_time = 0.0f;
static uint64_t event_counter = 0;
static uint64_t target_fps = 1000;

bool Engine::Init()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(5000);

    start_time = GetTime();

    return true;
}

void Engine::Run()
{
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        current_time = GetTime();

        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_UP))
        {
            target_fps += 1000;
            SetTargetFPS(target_fps);
        }
        if (IsKeyPressed(KEY_DOWN))
        {
            target_fps -= 1000;
            if (target_fps < 1000)
            {
                target_fps = 1000;
            }
            SetTargetFPS(target_fps);
        }

        input_manager.Update(GetTime());
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        EndDrawing();


        /* Check for events */

        uint64_t current_event_count = event_system.GetPendingEvents<UserInputEvent>();

        for (uint64_t i = 0; i < current_event_count; i++)
        {
            if (event_system.GetEvent<UserInputEvent>(i).has_value())
            {
                event_counter++;
            }
        }

        /* We check the events every second */
        if ((current_time - start_time) > 1.0f)
        {
            start_time = GetTime();
            std::cout << "Current FPS: " << GetFPS() << std::endl;
            std::cout << "Target FPS: " << target_fps << std::endl;
            std::cout << "Last Frame Time: " << GetFrameTime() << " seconds." << std::endl;
            std::cout << "Got " << event_counter << " Events this second." << std::endl;

            event_counter = 0;
        }


        event_system.Update();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();
}
