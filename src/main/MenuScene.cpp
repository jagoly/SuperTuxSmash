#include "main/MenuScene.hpp"

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/gl/Context.hpp>

using namespace sts;

//============================================================================//

MenuScene::MenuScene(SmashApp& smashApp)
    : Scene(1.0 / 60.0), mSmashApp(smashApp)
{
    mSmashApp.get_window().set_key_repeat(false);
}

MenuScene::~MenuScene() = default;

//============================================================================//

void MenuScene::refresh_options_destroy()
{
}

void MenuScene::refresh_options_create()
{
}

//============================================================================//

void MenuScene::handle_event(sq::Event /*event*/)
{
}

//============================================================================//

void MenuScene::update()
{
}

//============================================================================//

void MenuScene::integrate(double elapsed, float blend)
{

}

//============================================================================//

void MenuScene::render(double /*elapsed*/)
{
    auto& options = mSmashApp.get_options();
    auto& context = sq::Context::get();

    context.set_ViewPort(options.window_size);

    context.clear_depth_stencil_colour(1.0, 0x00, 0xFF, {0.f, 0.f, 0.f, 1.f});
}

//============================================================================//

void MenuScene::show_imgui_widgets()
{
    const auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowSizeConstraints({400, 0}, {400, -40});
    ImGui::SetNextWindowPos({20, 20});

    const auto window = ImPlus::ScopeWindow("Welcome to SuperTuxSmash", flags);
    if (window.show == false) return;

    //--------------------------------------------------------//

    ImPlus::ComboEnum("Stage", mSetup.stage);

    for (uint index = 0u; index < 4u; ++index)
    {
        auto& player = mSetup.players[index];

        if (ImPlus::CollapsingHeader("Player {}"_format(index+1)))
        {
            ImPlus::ComboEnum("Fighter##{}"_format(index), player.fighter);
            player.enabled = (player.fighter != FighterEnum::Null);
        }
    }

    //--------------------------------------------------------//

    if (ImPlus::Button("Start Game"))
    {
        const auto predicate = [](const auto& player) { return player.enabled; };
        const bool any = algo::any_of(mSetup.players, predicate);

        mSmashApp.start_game(any ? mSetup : GameSetup::get_defaults());
    }
    ImPlus::HoverTooltip("WARNING: Sara and Tux are usually broken, if you want a mostly working fighter use Mario (Quick Start)");

    ImGui::SameLine();

    if (ImPlus::Button("Start Action Editor"))
    {
        mSmashApp.start_action_editor();
    }

    ImGui::SameLine();

    if (ImPlus::Button("Quick Start"))
    {
        mSmashApp.start_game(GameSetup::get_quickstart());
    }
    ImPlus::HoverTooltip("currently this starts a game in TestZone with four Marios");
}

//============================================================================//

void MenuScene::populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf)
{
    const Vec2U windowSize = mSmashApp.get_window().get_size();

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            mSmashApp.get_window().get_render_pass(), framebuf, vk::Rect2D({0, 0}, {windowSize.x, windowSize.y})
        }, vk::SubpassContents::eInline
    );

    mSmashApp.get_gui_system().render_gui(cmdbuf);

    cmdbuf.endRenderPass();
}
