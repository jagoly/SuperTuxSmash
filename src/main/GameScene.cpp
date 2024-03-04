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

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Colours.hpp>
#include <sqee/vk/VulkanContext.hpp>

#include <ctime> // rng seed

using namespace sts;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp, GameSetup setup)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
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

    SQASSERT(setup.players.size() != 0u, "need at least one player");

    window.set_title(fmt::format(
        "SuperTuxSmash - {} - {}", setup.stage,
        fmt::join(views::transform(setup.players, [](auto& player) { return player.fighter; }), " vs. ")
    ));

    //--------------------------------------------------------//

    mRenderer = std::make_unique<Renderer>(window, options, resourceCaches);
    mWorld = std::make_unique<World>(options, audioContext, resourceCaches, *mRenderer);
    mWorld->set_rng_seed(uint_fast32_t(std::time(nullptr)));

    mStandardCamera = std::make_unique<StandardCamera>(*mRenderer);
    mEditorCamera = std::make_unique<EditorCamera>(*mRenderer);

    mRenderer->set_camera(*mStandardCamera);

    //--------------------------------------------------------//

    mWorld->create_stage(setup.stage);

    for (uint8_t index = 0u; index < setup.players.size(); ++index)
    {
        // todo: controllers should be created by MenuScene and trasferred to GameSetup
        auto& controller = mControllers.emplace_back (
            std::make_unique<Controller>(inputDevices, fmt::format("config/player{}.json", index+1u))
        );

        Fighter& fighter = mWorld->create_fighter(setup.players[index].fighter);
        fighter.controller = controller.get();
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
    mRenderer->swap_objects_buffers();

    // todo: pass the controller that paused the game
    mRenderer->get_camera().update_from_controller(*mControllers.front());

    mRenderer->integrate_camera(blend);
    mWorld->integrate(blend);

    mRenderer->integrate_particles(blend, mWorld->get_particle_system());
    mRenderer->integrate_debug(blend, *mWorld);

    if (mGamePaused == false)
    {
        for (auto& controller : mControllers)
            controller->refresh();

        mSmashApp.reset_inactivity(); // only go inactive if paused
    }

    mSmashApp.get_debug_overlay().update_sub_timers(mRenderer->get_frame_timings().data());
}

//============================================================================//

void GameScene::impl_show_general_window()
{
    ImGui::SetNextWindowSizeConstraints({300, -1}, {300, -1});
    ImGui::SetNextWindowPos({20, 20});

    const ImPlus::Scope_Window window = {
        "General Debug",
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar
    };
    if (window.show == false) return;

    auto& options = mSmashApp.get_options();

    //--------------------------------------------------------//

    ImPlus::if_MenuBar([&]()
    {
        ImPlus::if_Menu("Render", true, [&]()
        {
           ImGui::Checkbox("hit blobs", &options.render_hit_blobs);
           ImGui::Checkbox("hurt blobs", &options.render_hurt_blobs);
           ImGui::Checkbox("diamonds", &options.render_diamonds);
           ImGui::Checkbox("skeletons", &options.render_skeletons);
        });
        ImPlus::HoverTooltip("change debug rendering");

        ImPlus::if_Menu("Speed", true, [&]()
        {
            ImPlus::RadioButton("1fps", mTickTime, 1.0);
            ImPlus::RadioButton("0.125×", mTickTime, 1.0 / 6.0);
            ImPlus::RadioButton("0.25×", mTickTime, 1.0 / 12.0);
            ImPlus::RadioButton("0.5×", mTickTime, 1.0 / 24.0);
            ImPlus::RadioButton("1× (48fps)", mTickTime, 1.0 / 48.0);
            ImPlus::RadioButton("2×", mTickTime, 1.0 / 96.0);
        });
        ImPlus::HoverTooltip("change game speed");

        ImPlus::if_Menu("Log", true, [&]()
        {
            ImGui::Checkbox("animation", &options.log_animation);
            ImGui::Checkbox("script", &options.log_script);
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

    const ImPlus::Scope_Window window = {
        "Objects Debug",
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
    };
    if (window.show == false) return;

    //--------------------------------------------------------//

    DebugGui::show_widget_stage(mWorld->get_stage());

    for (auto& fighter : mWorld->get_fighters())
        DebugGui::show_widget_fighter(*fighter);

    for (auto& article : mWorld->get_articles())
        DebugGui::show_widget_article(*article);
}

//============================================================================//

void GameScene::impl_show_portraits()
{
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // todo: use a high resolution font and scale more smoothly
    const float fontScale = [&]() {
        const float availableWidth = displaySize.x * 0.8f;
        const float requiredWidth = 66.f * float(mWorld->get_fighters().size()) + 60.f;
        if (requiredWidth * 4.f < availableWidth) return 4.f;
        if (requiredWidth * 3.f < availableWidth) return 3.f;
        if (requiredWidth * 2.f < availableWidth) return 2.f;
        return 1.f;
    }();
    IMPLUS_WITH(Scope_FontScale) = { ImPlus::FONT_BOLD, fontScale };

    const float portraitWidth = 66.f * fontScale;

    const auto draw_text = [&](const String& text, Vec2F position, ImU32 colour)
    {
        drawList->AddText({position.x - fontScale, position.y + fontScale}, IM_COL32_BLACK, text);
        drawList->AddText({position.x, position.y}, IM_COL32_BLACK, text);
        drawList->AddText({position.x, position.y}, colour, text);
    };

    const auto draw_portrait = [&](const Fighter& fighter, float positionX)
    {
        // does nothing unless window width is not even
        positionX = std::floor(positionX);

        const ImVec2 rectMin = { positionX, displaySize.y - (18.f * fontScale) };
        const ImVec2 rectMax = { positionX + portraitWidth, displaySize.y };

        // slightly cyan to contrast with red damage
        drawList->AddRectFilled(rectMin, rectMax, IM_COL32(40, 80, 80, 96), 4.f * fontScale, ImDrawFlags_RoundCornersTop);

        const int damage = std::min(int(std::round(fighter.variables.damage)), 999);

        const String playerStr = { 'P', char('1' + fighter.index) };
        const String damageStr = fmt::format("{}%", damage);

        const ImU32 playerColour = [&]() {
            const auto colours = std::array {
                Vec3F(255, 38, 38), Vec3F(71, 83, 255), Vec3F(255, 187, 15), Vec3F(36, 159, 63)
            };
            const Vec3U rounded = Vec3U(maths::srgb_to_linear(colours[fighter.index] / 255.f) * 255.f + 0.5f);
            return IM_COL32(rounded.x, rounded.y, rounded.z, 255);
        }();

        const ImU32 damageColour = [&]() {
            const Vec3F colourA = maths::srgb_to_linear(Vec3F(255, 255, 255) / 255.f);
            const Vec3F colourB = maths::srgb_to_linear(Vec3F(255, 40, 40) / 255.f);
            const Vec3F colourC = maths::srgb_to_linear(Vec3F(40, 0, 0) / 255.f);
            const Vec3F mixed = damage <= 150 ?
                maths::mix(colourA, colourB, float(damage) / 150.f) :
                maths::mix(colourB, colourC, float(damage - 150) / 849.f);
            const Vec3U rounded = Vec3U(mixed * 255.f + 0.5f);
            return IM_COL32(rounded.x, rounded.y, rounded.z, 255);
        }();

        const float damageOffsetX = damage >= 100 ? 26.f : damage >= 10 ? 34.f : 42.f;

        draw_text(playerStr, {rectMin.x + 3.f * fontScale, rectMin.y}, playerColour);
        draw_text(damageStr, {rectMin.x + damageOffsetX * fontScale, rectMin.y}, damageColour);
    };

    if (mWorld->get_fighters().size() == 1u)
    {
        draw_portrait(*mWorld->get_fighters()[0], displaySize.x * 0.5f - portraitWidth * 0.5f);
    }
    else if (mWorld->get_fighters().size() == 2u)
    {
        const float spacing = 60.f * fontScale;
        draw_portrait(*mWorld->get_fighters()[0], displaySize.x * 0.5f - portraitWidth - spacing * 0.5f);
        draw_portrait(*mWorld->get_fighters()[1], displaySize.x * 0.5f + spacing * 0.5f);
    }
    else if (mWorld->get_fighters().size() == 3u)
    {
        const float spacing = 30.f * fontScale;
        draw_portrait(*mWorld->get_fighters()[0], displaySize.x * 0.5f - portraitWidth * 1.5f - spacing);
        draw_portrait(*mWorld->get_fighters()[1], displaySize.x * 0.5f - portraitWidth * 0.5f);
        draw_portrait(*mWorld->get_fighters()[2], displaySize.x * 0.5f + portraitWidth * 0.5f + spacing);
    }
    else if (mWorld->get_fighters().size() == 4u)
    {
        const float spacing = 20.f * fontScale;
        draw_portrait(*mWorld->get_fighters()[0], displaySize.x * 0.5f - portraitWidth * 2.f - spacing * 1.5f);
        draw_portrait(*mWorld->get_fighters()[1], displaySize.x * 0.5f - portraitWidth - spacing * 0.5f);
        draw_portrait(*mWorld->get_fighters()[2], displaySize.x * 0.5f + spacing * 0.5f);
        draw_portrait(*mWorld->get_fighters()[3], displaySize.x * 0.5f + portraitWidth + spacing * 1.5f);
    }
    else SQEE_UNREACHABLE();
}

//============================================================================//

void GameScene::show_imgui_widgets()
{
    impl_show_general_window();
    impl_show_objects_window();
    impl_show_portraits();
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
