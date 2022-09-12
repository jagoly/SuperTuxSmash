#include "main/MenuScene.hpp"

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

MenuScene::MenuScene(SmashApp& smashApp)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    mSmashApp.get_window().set_title("SuperTuxSmash - Main Menu");
    mSmashApp.get_debug_overlay().set_sub_timers({});

    // load stage names
    {
        const auto json = sq::parse_json_from_file("assets/stages/Stages.json");
        mStageNames.reserve(json.size());
        for (const auto& entry : json)
            mStageNames.emplace_back(entry.get_ref<const String&>());
    }

    // load fighter names
    {
        const auto json = sq::parse_json_from_file("assets/fighters/Fighters.json");
        mFighterNames.reserve(json.size());
        for (const auto& entry : json)
            mFighterNames.emplace_back(entry.get_ref<const String&>());
    }
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

void MenuScene::handle_event(sq::Event event)
{
    if (event.type == sq::Event::Type::Window_Close)
        mSmashApp.quit();
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
    ImPlus::ComboString("Stage", mStageNames, mSetup.stage);

    ImGui::Separator();

    for (auto iter = mSetup.players.begin(); iter != mSetup.players.end(); ++iter)
    {
        const auto index = uint8_t(iter - mSetup.players.begin());

        const bool erase = ImPlus::Button(fmt::format("X##{}", index));

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImPlus::Text(fmt::format("Player {}", index+1u));

        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        ImPlus::ComboString(fmt::format("Fighter##{}", index), mFighterNames, iter->fighter);

        if (erase) iter = mSetup.players.erase(iter) - 1;
    }

    if (mSetup.players.empty() == false)
        ImGui::Separator();

    //--------------------------------------------------------//

    // todo: get rid of defaults, we don't need two different quickstart buttons

    ImGui::Indent(50.f);
    if (ImPlus::Button("Start Game"))
    {
        if (mSetup.players.empty() == true && mSetup.stage.empty() == true)
            mSmashApp.start_game(GameSetup::get_defaults());

        bool valid = (mSetup.stage.empty() == false);
        for (auto& player : mSetup.players)
            valid &= (player.fighter.empty() == false);

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
