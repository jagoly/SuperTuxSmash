#include "main/GameScene.hpp"

#include "main/DebugGui.hpp"
#include "main/GameSetup.hpp"
#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include "game/Controller.hpp"
#include "game/Fighter.hpp"
#include "game/Stage.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"
#include "render/StandardCamera.hpp"

#include "editor/EditorCamera.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/vk/VulkanContext.hpp>

#include <ctime> // rng seed

using namespace sts;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp, GameSetup setup)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    // default to testzone, it's the only stage anyway
    if (setup.stage == StageEnum::Null) setup.stage = StageEnum::TestZone;

    mSmashApp.get_debug_overlay().set_sub_timers ({
        "BeginGbuffer", " Opaque", "EndGbuffer",
        "Shadows", "ShadowAverage", "DepthMipGen", "SSAO", "BlurSSAO",
        "BeginHDR", " Skybox", " LightDefault", " Transparent", " Particles", "EndHDR",
        "BeginFinal", " Composite", " Debug", " Gui", "EndFinal"
    });

    //--------------------------------------------------------//

    sq::Window& window = mSmashApp.get_window();
    sq::InputDevices& inputDevices = mSmashApp.get_input_devices();
    sq::AudioContext& audioContext = mSmashApp.get_audio_context();

    Options& options = mSmashApp.get_options();
    ResourceCaches& resourceCaches = mSmashApp.get_resource_caches();

    //--------------------------------------------------------//

    options.render_hit_blobs = false;
    options.render_hurt_blobs = false;
    options.render_diamonds = false;
    options.render_skeletons = false;

    //--------------------------------------------------------//

    window.set_key_repeat(false);

    String title = "SuperTuxSmash - {}"_format(setup.stage);

    for (uint8_t index = 0u; index < setup.players.size(); ++index)
        sq::format_append(title, index == 0u ? " - {}" : " vs. {}", setup.players[index].fighter);

    window.set_title(std::move(title));

    //--------------------------------------------------------//

    mRenderer = std::make_unique<Renderer>(window, options, resourceCaches);
    mWorld = std::make_unique<World>(false, options, audioContext, resourceCaches, *mRenderer);
    mWorld->set_rng_seed(uint_fast32_t(std::time(nullptr)));

    mStandardCamera = std::make_unique<StandardCamera>(*mRenderer);
    mEditorCamera = std::make_unique<EditorCamera>(*mRenderer);

    mRenderer->set_camera(*mStandardCamera);

    //--------------------------------------------------------//

    auto stage = std::make_unique<Stage>(*mWorld, setup.stage);

    mWorld->set_stage(std::move(stage));

    //--------------------------------------------------------//

    for (uint8_t index = 0u; index < setup.players.size(); ++index)
    {
        // todo: controllers should be created by MenuScene and trasferred to GameSetup
        auto& controller = mControllers.emplace_back (
            std::make_unique<Controller>(inputDevices, "config/player{}.json"_format(index+1u))
        );

        auto fighter = std::make_unique<Fighter>(*mWorld, setup.players[index].fighter, index);
        fighter->controller = controller.get();

        mWorld->add_fighter(std::move(fighter));
    }

    mWorld->finish_setup();
}

GameScene::~GameScene()
{
    sq::VulkanContext::get().device.waitIdle();
}

//============================================================================//

void GameScene::refresh_options_destroy()
{
    mRenderer->refresh_options_destroy();
}

void GameScene::refresh_options_create()
{
    mRenderer->refresh_options_create();
}

//============================================================================//

void GameScene::handle_event(sq::Event event)
{
    if (event.type == sq::Event::Type::Window_Close)
    {
        // todo: this should ask for confirmation
        mSmashApp.quit();
    }

    else if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::Escape)
        {
            // todo: this should open a pause menu
            mSmashApp.return_to_main_menu();
        }

        else if (event.data.keyboard.key == sq::Keyboard_Key::F1)
        {
            mGamePaused = !mGamePaused;
            mSmashApp.get_audio_context().set_groups_paused(sq::SoundGroup::Sfx, mGamePaused);
        }

        else if (event.data.keyboard.key == sq::Keyboard_Key::F2)
        {
            if (mGamePaused == true)
            {
                for (auto& controller : mControllers)
                    controller->refresh(), controller->tick();

                // advance by a single frame
                mWorld->tick();

                // todo: tell audio context to play one tick's worth of sound
            }
        }
    }
}

//============================================================================//

void GameScene::update()
{
    if (mGamePaused == false)
    {
        for (auto& controller : mControllers)
            controller->tick();

        mWorld->tick();
    }

    mRenderer->get_camera().update_from_world(*mWorld);
}

//============================================================================//

void GameScene::integrate(double /*elapsed*/, float blend)
{
    // todo: pass the controller that paused the game
    mRenderer->get_camera().update_from_controller(*mControllers.front());

    mRenderer->integrate_camera(blend);
    mWorld->integrate(blend);

    mRenderer->integrate_particles(blend, mWorld->get_particle_system());
    mRenderer->integrate_debug(blend, *mWorld);

    if (mGamePaused == false)
        for (auto& controller : mControllers)
            controller->refresh();

    mSmashApp.get_debug_overlay().update_sub_timers(mRenderer->get_frame_timings().data());
}

//============================================================================//

void GameScene::impl_show_general_window()
{
    ImGui::SetNextWindowSizeConstraints({300, -1}, {300, -1});
    ImGui::SetNextWindowPos({20, 20});

    const auto window = ImPlus::ScopeWindow {
        "General Debug",
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar
    };
    if (window.show == false) return;

    auto& options = mSmashApp.get_options();

    //--------------------------------------------------------//

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
            ImPlus::RadioButton("1fps", mTickTime, 1.0);
            ImPlus::RadioButton("0.125×", mTickTime, 1.0 / 6.0);
            ImPlus::RadioButton("0.25×", mTickTime, 1.0 / 12.0);
            ImPlus::RadioButton("0.5×", mTickTime, 1.0 / 24.0);
            ImPlus::RadioButton("1× (48fps)", mTickTime, 1.0 / 48.0);
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

    //--------------------------------------------------------//

    if (ImGui::Button("swap fighters"))
    {
        if (auto& fighters = mWorld->get_fighters(); fighters.size() >= 2u)
        {
            Controller* last = fighters.back()->controller;
            if (fighters.size() == 4u) fighters[3]->controller = fighters[2]->controller;
            if (fighters.size() >= 3u) fighters[2]->controller = fighters[1]->controller;
            fighters[1]->controller = fighters[0]->controller;
            fighters[0]->controller = last;
        }
    }
    ImPlus::HoverTooltip("cycle the controllers");

    ImGui::SameLine();

    if (&mRenderer->get_camera() == mStandardCamera.get())
    {
        if (ImGui::Button("camera: standard")) mRenderer->set_camera(*mEditorCamera);
        ImPlus::HoverTooltip("change to editor camera");
    }
    else if (&mRenderer->get_camera() == mEditorCamera.get())
    {
        if (ImGui::Button("camera: editor")) mRenderer->set_camera(*mStandardCamera);
        ImPlus::HoverTooltip("change to standard camera");
    }

    // todo: give each camera a show_debug_widget method
    ImGui::Checkbox("smooth", &options.camera_smooth);
    ImPlus::HoverTooltip("smooth camera movement");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImPlus::SliderValue("##zoom", options.camera_zoom_out, 0.5f, 2.f, "zoom out: %.2f");
    options.camera_zoom_out = std::round(options.camera_zoom_out * 4.f) * 0.25f;
}

//============================================================================//

void GameScene::impl_show_objects_window()
{
    ImGui::SetNextWindowSizeConstraints({360, 0}, {360, ImPlus::FromScreenBottom(20+20)});
    ImGui::SetNextWindowPos({ImPlus::FromScreenRight(360+20), 20});

    const auto window = ImPlus::ScopeWindow {
        "Objects Debug",
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove
    };
    if (window.show == false) return;

    //--------------------------------------------------------//

    DebugGui::show_widget_stage(mWorld->get_stage());

    for (auto& fighter : mWorld->get_fighters())
        DebugGui::show_widget_fighter(*fighter);
}

//============================================================================//

void GameScene::show_imgui_widgets()
{
    impl_show_general_window();
    impl_show_objects_window();
}

//============================================================================//

void GameScene::populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf)
{
    const Vec2U windowSize = mSmashApp.get_window().get_size();

    mRenderer->populate_command_buffer(cmdbuf);

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            mSmashApp.get_window().get_render_pass(), framebuf, vk::Rect2D({0, 0}, {windowSize.x, windowSize.y})
        }, vk::SubpassContents::eInline
    );
    mRenderer->write_time_stamp(cmdbuf, Renderer::TimeStamp::BeginFinal);

    mRenderer->populate_final_pass(cmdbuf);

    mSmashApp.get_gui_system().render_gui(cmdbuf);
    mRenderer->write_time_stamp(cmdbuf, Renderer::TimeStamp::Gui);

    cmdbuf.endRenderPass();
    mRenderer->write_time_stamp(cmdbuf, Renderer::TimeStamp::EndFinal);
}
