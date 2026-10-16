#include "GravityTest.h"
#include "Imager.h"
#include "SceneSwitches.h"

#include <random>

using namespace Mupfel;

std::mt19937 rng;

void GravityTest::OnInit()
{
    std::random_device rd;
    rng.seed(rd());

    const Entity ground = Entities::Create();
    Transform t;
    t.pos_y = -8.0f;
    Entities::AddComponent<Transform>(ground, t);
    Entities::AddComponent<Body>(ground, {});
    Collider c;
    c.SetBox(30, 0.4);
    Entities::AddComponent<Collider>(ground, c);
    const Entity side_right = Entities::Create();
    t.pos_x = 15.2f;
    t.pos_y = -1.2f;
    Entities::AddComponent<Transform>(side_right, t);
    c.SetBox(0.4, 14);
    Entities::AddComponent<Collider>(side_right, c);
    Entities::AddComponent<Body>(side_right, {});
    const Entity side_left = Entities::Create();
    t.pos_x = -15.2f;
    t.pos_y = -1.2f;
    Entities::AddComponent<Transform>(side_left, t);
    c.SetBox(0.4, 14);
    Entities::AddComponent<Collider>(side_left, c);
    Entities::AddComponent<Body>(side_left, {});

    /* Add some balls */
    Imager::Load("Ball", "Images/ball_blue.png");
}

void GravityTest::OnUpdate(double timestep)
{
    static double spawn_timer = 0.0f;
    static uint32_t total_balls = 0;
    static constexpr uint32_t max_balls = 4000;
    static std::uniform_real_distribution<float> dist_position(-9.0f, 9.0f);
    static std::uniform_real_distribution<float> dist_collider_size(0.05f, 0.1f);

    spawn_timer += timestep;

    /* Spawn 500 balls per second (max, depends on the frame rate) */
    if (spawn_timer > 0.005f && total_balls < max_balls)
    {
        spawn_timer = 0.0f;

        const float pos_x = dist_position(rng);
        const float collider_size = dist_collider_size(rng);

        const Entity e = Entities::Create();
        Entities::AddComponent<Transform>(e, {.pos_x = pos_x, .pos_y = 7.0f});
        Entities::AddComponent<Body>(e, {.type = BodyType::Dynamic});
        Collider c;
        c.SetCircle(collider_size);
        Entities::AddComponent<Collider>(e, c);
        Entities::AddComponent<Texture>(e, {
                                            .index = Imager::Get("Ball"),
                                            .scale_x = collider_size * 2,
                                            .scale_y = collider_size * 2
                                        });

        total_balls++;
    }
}

void GravityTest::OnRender()
{
    /* Draw a Button */
    if (UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/main_menu.png") == 3)
    {
        logger->info("Switching to the Main Menu...");
        Events::Post<SwitchSceneEvent>(SwitchSceneEvent(AppScene::MAIN_MENU));
    }
}
