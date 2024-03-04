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
        const auto document = JsonDocument::parse_file("assets/stages/Stages.json");
        const auto json = document.root().as<JsonArray>();
        mStageNames.reserve(json.size());
        for (const auto [_, name] : json)
            mStageNames.emplace_back(name.as<String>());
    }

    // load fighter names
    {
        const auto document = JsonDocument::parse_file("assets/fighters/Fighters.json");
        const auto json = document.root().as<JsonArray>();
        mFighterNames.reserve(json.size());
        for (const auto [_, name] : json)
            mFighterNames.emplace_back(name.as<String>());
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

    const ImPlus::Scope_Window window = {
        " Welcome to SuperTuxSmash!",
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
    };
    if (window.show == false) return;

    //--------------------------------------------------------//

    if (ImGui::Button("Add Player") && mSetup.players.size() < MAX_FIGHTERS)
        mSetup.players.emplace_back();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.f);
    ImPlus::ComboString("Stage", mStageNames, mSetup.stage);

    ImGui::Separator();

    for (auto iter = mSetup.players.begin(); iter != mSetup.players.end();)
    {
        const auto index = uint8_t(iter - mSetup.players.begin());

        const bool erase = ImGui::Button(fmt::format("X##{}", index));

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImPlus::Text(fmt::format("Player {}", index+1u));

        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        ImPlus::ComboString(fmt::format("Fighter##{}", index), mFighterNames, iter->fighter);

        if (erase) iter = mSetup.players.erase(iter);
        else ++iter;
    }

    if (mSetup.players.empty() == false)
        ImGui::Separator();

    //--------------------------------------------------------//

    // todo: get rid of defaults, we don't need two different quickstart buttons

    ImGui::Indent(50.f);
    if (ImGui::Button("Start Game"))
    {
        if (mSetup.stage.empty())
            mSmashApp.get_debug_overlay().notify("Please choose a Stage");
        else if (mSetup.players.empty())
            mSmashApp.get_debug_overlay().notify("Please add some Players");
        else
        {
            auto undecided = ranges::find_if(mSetup.players, [](auto& player) { return player.fighter.empty(); });
            if (undecided != mSetup.players.end())
            {
                auto pNum = std::distance(mSetup.players.begin(), undecided) + 1;
                mSmashApp.get_debug_overlay().notify(fmt::format("Please choose a Fighter for Player {}", pNum));
            }
            else mSmashApp.start_game(mSetup);
        }
    }
    ImPlus::HoverTooltip("WARNING: Sara and Tux are usually broken, if you want a mostly working fighter use Mario (Quick Start)");

    ImGui::SameLine();
    if (ImGui::Button("Start Editor"))
    {
        mSmashApp.start_editor();
    }

    ImGui::SameLine();
    if (ImGui::Button("Quick Start"))
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
