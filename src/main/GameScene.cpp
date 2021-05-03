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

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Text.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp, GameSetup setup)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    // default to testzone, it's the only stage anyway
    if (setup.stage == StageEnum::Null) setup.stage = StageEnum::TestZone;

    //--------------------------------------------------------//

    sq::VulkWindow& window = mSmashApp.get_window();
    sq::VulkInputDevices& inputDevices = mSmashApp.get_input_devices();
    sq::AudioContext& audioContext = mSmashApp.get_audio_context();

    Options& options = mSmashApp.get_options();
    ResourceCaches& resourceCaches = mSmashApp.get_resource_caches();

    //--------------------------------------------------------//

    options.render_hit_blobs = false;
    options.render_hurt_blobs = false;
    options.render_diamonds = false;
    options.render_skeletons = false;

    options.editor_mode = false;

    //--------------------------------------------------------//

    window.set_key_repeat(false);

    String title = "SuperTuxSmash - {}"_format(setup.stage);

    if (setup.players[0].enabled == true)
        title += " - {}"_format(setup.players[0].fighter);

    for (uint8_t index = 1u; index < MAX_FIGHTERS; ++index)
        if (setup.players[index].enabled)
            title += " vs. {}"_format(setup.players[index].fighter);

    window.set_title(std::move(title));

    //--------------------------------------------------------//

    mRenderer = std::make_unique<Renderer>(window, options, resourceCaches);
    mFightWorld = std::make_unique<FightWorld>(options, audioContext, resourceCaches, *mRenderer);

    mRenderer->set_camera(std::make_unique<StandardCamera>(*mRenderer));

    for (uint8_t index = 0u; index < MAX_FIGHTERS; ++index)
        mControllers[index] = std::make_unique<Controller>(inputDevices, "config/player{}.json"_format(index+1u));

    //--------------------------------------------------------//

    auto stage = std::make_unique<Stage>(*mFightWorld, setup.stage);

    mFightWorld->set_stage(std::move(stage));

    //--------------------------------------------------------//

    for (uint8_t index = 0u; index < MAX_FIGHTERS; ++index)
    {
        if (setup.players[index].enabled == false) continue;

        auto fighter = std::make_unique<Fighter>(*mFightWorld, setup.players[index].fighter, index);

        fighter->set_controller(mControllers[index].get());

        mFightWorld->add_fighter(std::move(fighter));
    }

    mFightWorld->finish_setup();
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
    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::F1)
        {
            mGamePaused = !mGamePaused;
            mSmashApp.get_audio_context().set_groups_paused(sq::SoundGroup::Sfx, mGamePaused);
            return;
        }

        if (event.data.keyboard.key == sq::Keyboard_Key::F2)
        {
            if (mGamePaused == true)
            {
                // todo: tell audio context to play one tick's worth of sound
                mFightWorld->tick();
            }
            return;
        }
    }

    for (auto& controller : mControllers)
    {
        if (controller != nullptr)
        {
            if (mGamePaused == false)
                controller->handle_event(event);
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

void GameScene::integrate(double /*elapsed*/, float blend)
{
    mFightWorld->integrate(blend);
    mRenderer->integrate(blend);

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
}

//============================================================================//

void GameScene::render(double /*elapsed*/)
{
    /*mRenderer->render_particles(mFightWorld->get_particle_system(), blend);

    String damageString; damageString.reserve(47u);

    for (const Fighter* fighter : mFightWorld->get_fighters())
    {
        const int damage = int(std::round(fighter->status.damage));
        damageString.clear();
        damageString += "P{}: {}%  "_format(fighter->index + 1u, damage);
        if (damage < 100) damageString += ' ';
        if (damage < 10) damageString += ' ';
        for (size_t i = mFightWorld->get_fighters().size() - 1u; i > fighter->index; --i) damageString += "           ";
        Vec4F damageColour = { 1.f, 1.f, 1.f, 1.f };
        if (damage >= 25) damageColour = { 1.f, 1.f, 0.6f, 1.f };
        if (damage >= 55) damageColour = { 1.f, 0.9f, 0.3f, 1.f };
        if (damage >= 90) damageColour = { 1.f, 0.3f, 0.3f, 1.f };
        sq::render_text_basic(damageString, {+1, +1}, {+1, -1}, {28.f, 40.f}, damageColour, true);
    }*/
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

    ImGui::SetNextItemWidth(-1.f);
    ImPlus::SliderValue("##zoom", options.camera_zoom_out, 0.5f, 2.f, "zoom out: %.2f");
    options.camera_zoom_out = std::round(options.camera_zoom_out * 4.f) * 0.25f;
}

//============================================================================//

void GameScene::impl_show_fighters_window()
{
    const auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowSizeConstraints({360, 0}, {360, ImPlus::FromScreenBottom(20+20)});
    ImGui::SetNextWindowPos({ImPlus::FromScreenRight(360+20), 20});

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

    mRenderer->populate_final_pass(cmdbuf);

    mSmashApp.get_gui_system().render_gui(cmdbuf);

    cmdbuf.endRenderPass();
}
