#include "main/GameScene.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"
#include "fighters/Mario_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"
#include "fighters/Mario_Render.hpp"

#include "render/DebugRender.hpp"

#include "main/DebugGui.hpp"

#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp, GameSetup setup)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    mGeneralWidget.func = [this]() { impl_show_general_window(); };
    mFightersWidget.func = [this]() { impl_show_fighters_window(); };

    mSmashApp.get_window().set_key_repeat(false);

    // todo: these "globals" are dumb and not really needed, clean it up

    smashApp.get_globals().renderHitBlobs = false;
    smashApp.get_globals().renderHurtBlobs = false;
    smashApp.get_globals().renderDiamonds = false;
    smashApp.get_globals().renderSkeletons = false;

    smashApp.get_globals().editorMode = false;

    //--------------------------------------------------------//

    mFightWorld = std::make_unique<FightWorld>(mSmashApp.get_globals());
    mRenderer = std::make_unique<Renderer>(mSmashApp.get_globals(), mSmashApp.get_options());

    //--------------------------------------------------------//

    mControllers[0] = std::make_unique<Controller>(mSmashApp.get_globals(), mSmashApp.get_input_devices(), "player1.json");
    mControllers[1] = std::make_unique<Controller>(mSmashApp.get_globals(), mSmashApp.get_input_devices(), "player2.json");
    mControllers[2] = std::make_unique<Controller>(mSmashApp.get_globals(), mSmashApp.get_input_devices(), "player3.json");
    mControllers[3] = std::make_unique<Controller>(mSmashApp.get_globals(), mSmashApp.get_input_devices(), "player4.json");

    //--------------------------------------------------------//

    UniquePtr<Stage> stage;
    UniquePtr<RenderObject> renderStage;

    SWITCH (setup.stage)
    {
        //CASE (TestZone)
        CASE (Null, TestZone)
        {
            stage = std::make_unique<TestZone_Stage>(*mFightWorld);
            renderStage = std::make_unique<TestZone_Render>(*mRenderer, static_cast<TestZone_Stage&>(*stage));
        }

        CASE_DEFAULT SQASSERT(false, "bad stage setup");
    }
    SWITCH_END;

    mFightWorld->set_stage(std::move(stage));
    mRenderer->add_object(std::move(renderStage));

    //--------------------------------------------------------//

    for (uint8_t index = 0u; index < 4u; ++index)
    {
        if (setup.players[index].enabled == false) continue;

        UniquePtr<Fighter> fighter;
        UniquePtr<RenderObject> renderFighter;

        SWITCH (setup.players[index].fighter)
        {
            CASE (Sara)
            {
                fighter = std::make_unique<Sara_Fighter>(index, *mFightWorld);
                renderFighter = std::make_unique<Sara_Render>(*mRenderer, static_cast<Sara_Fighter&>(*fighter));
            }

            CASE (Tux)
            {
                fighter = std::make_unique<Tux_Fighter>(index, *mFightWorld);
                renderFighter = std::make_unique<Tux_Render>(*mRenderer, static_cast<Tux_Fighter&>(*fighter));
            }

            CASE (Mario)
            {
                fighter = std::make_unique<Mario_Fighter>(index, *mFightWorld);
                renderFighter = std::make_unique<Mario_Render>(*mRenderer, static_cast<Mario_Fighter&>(*fighter));
            }

            CASE_DEFAULT SQASSERT(false, "bad fighter setup");
        }
        SWITCH_END;

        fighter->set_controller(mControllers[index].get());

        mFightWorld->add_fighter(std::move(fighter));
        mRenderer->add_object(std::move(renderFighter));
    }
}

GameScene::~GameScene() = default;

//============================================================================//

void GameScene::refresh_options()
{
    mRenderer->refresh_options();
}

//============================================================================//

void GameScene::handle_event(sq::Event event)
{
    for (auto& controller : mControllers)
    {
        if (mGamePaused == false)
        {
            if (controller != nullptr)
                controller->handle_event(event);
        }
    }

    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::F1)
            mGamePaused = !mGamePaused;

        if (mGamePaused == true)
        {
            if (event.data.keyboard.key == sq::Keyboard_Key::F2)
                mFightWorld->tick();
        }
    }
}

//============================================================================//

void GameScene::update()
{
    if (mGamePaused == false) mFightWorld->tick();

    auto& camera = static_cast<StandardCamera&>(mRenderer->get_camera());
    camera.update_from_scene_data(mFightWorld->compute_scene_data());
}

//============================================================================//

void GameScene::render(double elapsed)
{
    const float accum = float(elapsed);
    const float blend = float(mAccumulation / mTickTime);

    mRenderer->render_objects(accum, blend);

    mRenderer->resolve_multisample();

    mRenderer->render_particles(mFightWorld->get_particle_system(), accum, blend);

    auto& debugRenderer = mRenderer->get_debug_renderer();

    if (mSmashApp.get_globals().renderHitBlobs == true)
        debugRenderer.render_hit_blobs(mFightWorld->get_hit_blobs());

    if (mSmashApp.get_globals().renderHurtBlobs == true)
        debugRenderer.render_hurt_blobs(mFightWorld->get_hurt_blobs());

    if (mSmashApp.get_globals().renderDiamonds == true)
        for (const auto fighter : mFightWorld->get_fighters())
            debugRenderer.render_diamond(fighter->get_position(), fighter->diamond);

    if (mSmashApp.get_globals().renderSkeletons == true)
        for (const auto fighter : mFightWorld->get_fighters())
            debugRenderer.render_skeleton(*fighter);

    mRenderer->finish_rendering();
}

//============================================================================//

void GameScene::impl_show_general_window()
{
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;
    ImGui::SetNextWindowSizeConstraints({300, 0}, {300, 200});
    ImGui::SetNextWindowPos({20, 20});

    const ImPlus::ScopeWindow window = { "General Debug", flags };
    if (window.show == false) return;

    //--------------------------------------------------------//

    auto& globals = mSmashApp.get_globals();

    if (ImGui::Button("reload actions"))
    {
        for (Fighter* fighter : mFightWorld->get_fighters())
            fighter->debug_reload_actions();
    }
    ImPlus::HoverTooltip("reload action from json");

    ImGui::SameLine();

    if (ImGui::Button("swap control"))
    {
        if (auto fighters = mFightWorld->get_fighters(); fighters.size() >= 2u)
        {
            auto controllerLast = fighters.back()->get_controller();
            if (fighters.size() == 4u) fighters[3]->set_controller(fighters[2]->get_controller());
            if (fighters.size() >= 3u) fighters[2]->set_controller(fighters[1]->get_controller());
            fighters[1]->set_controller(fighters[0]->get_controller());
            fighters[0]->set_controller(controllerLast);
        }
    }
    ImPlus::HoverTooltip("cycle the controllers");

    ImGui::Checkbox("disable input", &globals.disableInput);
    ImPlus::HoverTooltip("disable game input completely");

    ImPlus::if_MenuBar([&]()
    {
        ImPlus::if_Menu("render...", true, [&]()
        {
           ImPlus::Checkbox("hit blobs", &globals.renderHitBlobs);
           ImPlus::Checkbox("hurt blobs", &globals.renderHurtBlobs);
           ImPlus::Checkbox("diamonds", &globals.renderDiamonds);
           ImPlus::Checkbox("skeletons", &globals.renderSkeletons);
        });
        ImPlus::HoverTooltip("change debug rendering");

        ImPlus::if_Menu("speed...", true, [&]()
        {
            ImPlus::RadioButton("0.03125×", mTickTime, 1.0 / 1.5);
            ImPlus::RadioButton("0.125×", mTickTime, 1.0 / 6.0);
            ImPlus::RadioButton("0.5×", mTickTime, 1.0 / 24.0);
            ImPlus::RadioButton("1×", mTickTime, 1.0 / 48.0);
            ImPlus::RadioButton("2×", mTickTime, 1.0 / 96.0);
        });
        ImPlus::HoverTooltip("change game speed");
    });

    float& zoomRef = static_cast<StandardCamera&>(mRenderer->get_camera()).zoomOut;
    ImGui::SetNextItemWidth(-1.f);
    ImPlus::SliderValue("##zoom", zoomRef, -1.f, 8.f, "zoom: %.2f");
    zoomRef = std::round(zoomRef * 2.f) * 0.5f;
}

//============================================================================//

void GameScene::impl_show_fighters_window()
{
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowSizeConstraints({420, 0}, {420, ImPlus::FromScreenBottom(20+20)});
    ImGui::SetNextWindowPos({ImPlus::FromScreenRight(420+20), 20});

    if (ImGui::Begin("Fighter Debug", nullptr, flags))
    {
        //--------------------------------------------------------//

        for (Fighter* fighter : mFightWorld->get_fighters())
            DebugGui::show_widget_fighter(*fighter);
    }

    ImGui::End();
}
