#include "main/GameScene.hpp"

#include "main/DebugGui.hpp"
#include "main/GameSetup.hpp"
#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/Stage.hpp"

#include "render/DebugRender.hpp"
#include "render/Renderer.hpp"
#include "render/StandardCamera.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"
#include "fighters/Mario_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"
#include "fighters/Mario_Render.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp, GameSetup setup)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    auto& options = mSmashApp.get_options();

    options.render_hit_blobs = false;
    options.render_hurt_blobs = false;
    options.render_diamonds = false;
    options.render_skeletons = false;

    options.editor_mode = false;

    auto& window = mSmashApp.get_window();

    window.set_key_repeat(false);

    String title = "SuperTuxSmash - {}"_format(setup.stage);

    if (setup.players[0].enabled == true)
        title += " - {}"_format(setup.players[0].fighter);

    for (uint8_t index = 1u; index < MAX_FIGHTERS; ++index)
        if (setup.players[index].enabled)
            title += " vs. {}"_format(setup.players[index].fighter);

    window.set_window_title(std::move(title));

    //--------------------------------------------------------//

    mFightWorld = std::make_unique<FightWorld>(options, mSmashApp.get_audio_context());
    mRenderer = std::make_unique<Renderer>(options);

    mRenderer->set_camera(std::make_unique<StandardCamera>(*mRenderer));

    auto& inputDevices = mSmashApp.get_input_devices();

    for (uint8_t index = 0u; index < MAX_FIGHTERS; ++index)
        mControllers[index] = std::make_unique<Controller>(inputDevices, "config/player{}.json"_format(index+1u));

    //--------------------------------------------------------//

    std::unique_ptr<Stage> stage;
    std::unique_ptr<RenderObject> renderStage;

    SWITCH (setup.stage)
    {
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

    for (uint8_t index = 0u; index < MAX_FIGHTERS; ++index)
    {
        if (setup.players[index].enabled == false) continue;

        std::unique_ptr<Fighter> fighter;
        std::unique_ptr<RenderObject> renderFighter;

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

    mFightWorld->finish_setup();
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
        {
            mGamePaused = !mGamePaused;
            mSmashApp.get_audio_context().set_groups_paused(sq::SoundGroup::Sfx, mGamePaused);
        }

        if (mGamePaused == true)
        {
            // todo: what should sound do here?
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
    camera.update_from_world(*mFightWorld);
}

//============================================================================//

void GameScene::render(double /*elapsed*/)
{
    const float blend = float(mAccumulation / mTickTime);

    mRenderer->render_objects(blend);

    mRenderer->resolve_multisample();

    mRenderer->render_particles(mFightWorld->get_particle_system(), blend);

    auto& options = mSmashApp.get_options();
    auto& debugRenderer = mRenderer->get_debug_renderer();

    if (options.render_hit_blobs == true)
        debugRenderer.render_hit_blobs(mFightWorld->get_hit_blobs());

    if (options.render_hurt_blobs == true)
        debugRenderer.render_hurt_blobs(mFightWorld->get_hurt_blobs());

    if (options.render_diamonds == true)
        for (const auto fighter : mFightWorld->get_fighters())
            debugRenderer.render_diamond(*fighter);

    if (options.render_skeletons == true)
        for (const auto fighter : mFightWorld->get_fighters())
            debugRenderer.render_skeleton(*fighter);

    mRenderer->finish_rendering();
}

//============================================================================//

void GameScene::impl_show_general_window()
{
    const auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;
    ImGui::SetNextWindowSizeConstraints({300, 0}, {300, 200});
    ImGui::SetNextWindowPos({20, 20});

    const ImPlus::ScopeWindow window = { "General Debug", flags };
    if (window.show == false) return;

    //--------------------------------------------------------//

    auto& options = mSmashApp.get_options();

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

    ImGui::Checkbox("disable input", &options.input_disable);
    ImPlus::HoverTooltip("disable game input completely");

    ImGui::SameLine();

    ImGui::Checkbox("smooth camera", &options.camera_smooth);
    ImPlus::HoverTooltip("smooth camera movement");

    ImPlus::if_MenuBar([&]()
    {
        ImPlus::if_Menu("render...", true, [&]()
        {
           ImPlus::Checkbox("hit blobs", &options.render_hit_blobs);
           ImPlus::Checkbox("hurt blobs", &options.render_hurt_blobs);
           ImPlus::Checkbox("diamonds", &options.render_diamonds);
           ImPlus::Checkbox("skeletons", &options.render_skeletons);
        });
        ImPlus::HoverTooltip("change debug rendering");

        ImPlus::if_Menu("speed...", true, [&]()
        {
            ImPlus::RadioButton("0.03125×", mTickTime, 1.0 / 1.5);
            ImPlus::RadioButton("0.125×", mTickTime, 1.0 / 6.0);
            ImPlus::RadioButton("0.25×", mTickTime, 1.0 / 12.0);
            ImPlus::RadioButton("0.5×", mTickTime, 1.0 / 24.0);
            ImPlus::RadioButton("1×", mTickTime, 1.0 / 48.0);
            ImPlus::RadioButton("2×", mTickTime, 1.0 / 96.0);
        });
        ImPlus::HoverTooltip("change game speed");

        ImPlus::if_Menu("log...", true, [&]()
        {
            ImPlus::Checkbox("animation", &options.log_animation);
            ImPlus::Checkbox("script", &options.log_script);
        });
        ImPlus::HoverTooltip("change debug logging");
    });

    ImGui::SetNextItemWidth(-1.f);
    ImPlus::SliderValue("##zoom", options.camera_zoom_out, 0.5f, 2.f, "zoom out: %.2f");
    options.camera_zoom_out = std::round(options.camera_zoom_out * 4.f) * 0.25f;
}

//============================================================================//

void GameScene::impl_show_fighters_window()
{
    const auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowSizeConstraints({420, 0}, {420, ImPlus::FromScreenBottom(20+20)});
    ImGui::SetNextWindowPos({ImPlus::FromScreenRight(420+20), 20});

    const ImPlus::ScopeWindow window = { "Fighter Debug", flags };
    if (window.show == false) return;

    //--------------------------------------------------------//

    for (Fighter* fighter : mFightWorld->get_fighters())
        DebugGui::show_widget_fighter(*fighter);
}

//============================================================================//

void GameScene::show_imgui_widgets()
{
    impl_show_general_window();
    impl_show_fighters_window();
}
