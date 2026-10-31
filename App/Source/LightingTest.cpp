#include "LightingTest.h"
#include "Events/UserInputEvent.h"

#include "imgui.h"

using namespace Mupfel;

void LightingTest::OnInit()
{
    Input::MapKey<UserAction>(Key::KEY_SPACE, RELEASED, UserAction{UserAction::Action::GENERATE_ENTITY});
    texture = Images::LoadAnimated("Images/SpaceInvaders/simple_invader_animated.png",
                                   {.rows = 3, .columns = 8}).value_or(INVALID_HANDLE);
}

void LightingTest::OnUpdate(double timestep)
{
    (void)timestep;
    for ([[maybe_unused]] const auto& event : Events::Get<UserAction>())
    {
        /* Generate a default entity at cursor location. */
        const auto cursor_x = static_cast<float>(Application::GetCurrentInputManager().GetCurrentCursorX());
        const auto cursor_y = static_cast<float>(Application::GetCurrentInputManager().GetCurrentCursorY());

        if (auto optPoint = camera.ScreenToWorld(cursor_x, cursor_y))
        {
            auto [x, y, z] = optPoint.value();

            const Entity e = Entities::Create();

            Entities::AddComponent<Transform>(e, {.pos_x = x, .pos_y = y, .pos_z = z});
            Entities::AddComponent<Texture>(e, {.index = texture, .scale_x = 0.8f, .scale_y = 0.8f});
        }
    }
}

void LightingTest::OnRender()
{
    if (ImGui::Begin("My Tools"))
    {
        static float speed = 0.0f;
        ImGui::SliderFloat("Speed", &speed, 0.0f, 10.0f);
    }
    ImGui::End();
}

void LightingTest::UpdateUserInputs()
{
    static bool movable_camera = false;
    static ScreenPoint last_cursor_on_screen = {
        static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())
    };
    static float last_camera_x = camera.target_x;
    static float last_camera_y = camera.target_y;
    /* First, check mouse movement */
    for (auto& event : Events::Get<UserInputEvent>())
    {
        if (event.input == UserInput::RIGHT_MOUSE_CLICK && event.action == PRESSED)
        {
            /* Mouse button has been pressed this frame */
            movable_camera = true;
            last_cursor_on_screen.x = Input::CursorX();
            last_cursor_on_screen.y = Input::CursorY();
        }

        if (event.input == UserInput::RIGHT_MOUSE_CLICK && event.action == RELEASED)
        {
            movable_camera = false;
            /* Mouse button has been pressed this frame */
            const ScreenPoint screen_cursor = {
                static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())
            };

            const ScreenVector diff = {
                screen_cursor.x - last_cursor_on_screen.x, screen_cursor.y - last_cursor_on_screen.y
            };

            auto world_diff = camera.ScreenToWorldVector(diff.x, diff.y);

            if (world_diff)
            {
                last_camera_x = last_camera_x - world_diff.value().x;
                last_camera_y = last_camera_y - world_diff.value().y;
            }
        }

        if (event.input == UserInput::SCROLLWHEEL_UP)
        {
            if (camera.distance > 1.0f)
                camera.distance -= 1.0f;
        }

        if (event.input == UserInput::SCROLLWHEEL_DOWN)
        {
            camera.distance += 1.0f;
        }
    }

    /* Update the camera if needed. */
    if (movable_camera)
    {
        ScreenPoint screen_cursor = {static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())};

        ScreenVector diff = {screen_cursor.x - last_cursor_on_screen.x, screen_cursor.y - last_cursor_on_screen.y};

        auto world_diff = camera.ScreenToWorldVector(diff.x, diff.y);

        if (world_diff)
        {
            camera.target_x = last_camera_x - world_diff.value().x;
            camera.target_y = last_camera_y - world_diff.value().y;
        }
    }
}
