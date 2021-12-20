#include "main/MenuScene.hpp"

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

MenuScene::MenuScene(SmashApp& smashApp)
    : Scene(1.0 / 60.0), mSmashApp(smashApp)
{
    mSmashApp.get_window().set_key_repeat(false);

    mSmashApp.get_debug_overlay().set_sub_timers({});
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

void MenuScene::integrate(double /*elapsed*/, float /*blend*/)
{

}

//============================================================================//

void MenuScene::show_imgui_widgets()
{
    ImGui::SetNextWindowPos({20, 20});

    const auto window = ImPlus::ScopeWindow (
        " Welcome to SuperTuxSmash",
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
    );
    if (window.show == false) return;

    //--------------------------------------------------------//

    if (ImGui::Button("Add Player") && mSetup.players.size() < MAX_FIGHTERS)
        mSetup.players.emplace_back();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.f);
    ImPlus::ComboEnum("Stage", mSetup.stage);

    ImGui::Separator();

    for (auto iter = mSetup.players.begin(); iter != mSetup.players.end(); ++iter)
    {
        const auto index = uint8_t(iter - mSetup.players.begin());

        const bool erase = ImPlus::Button("X##{}"_format(index));

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImPlus::Text("Player {}"_format(index + 1));

        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        ImPlus::ComboEnum("Fighter##{}"_format(index), iter->fighter);

        if (erase) iter = mSetup.players.erase(iter) - 1;
    }

    if (mSetup.players.empty() == false)
        ImGui::Separator();

    //--------------------------------------------------------//

    ImGui::Indent(50.f);
    if (ImPlus::Button("Start Game"))
    {
        if (mSetup.players.empty() == true && mSetup.stage == StageEnum::Null)
            mSmashApp.start_game(GameSetup::get_defaults());

        bool valid = (mSetup.stage != StageEnum::Null);
        for (auto& player : mSetup.players)
            valid &= (player.fighter != FighterEnum::Null);

        if (valid == true)
             mSmashApp.start_game(mSetup);
    }
    ImPlus::HoverTooltip("WARNING: Sara and Tux are usually broken, if you want a mostly working fighter use Mario (Quick Start)");

    ImGui::SameLine();
    if (ImPlus::Button("Start Editor"))
    {
        mSmashApp.start_editor();
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

    cmdbuf.clearAttachments (
        vk::ClearAttachment(vk::ImageAspectFlagBits::eColor, 0u, vk::ClearValue(vk::ClearColorValue().setFloat32({}))),
        vk::ClearRect(vk::Rect2D({0, 0}, {windowSize.x, windowSize.y}), 0u, 1u)
    );

    mSmashApp.get_gui_system().render_gui(cmdbuf);

    cmdbuf.endRenderPass();
}
