#include "main/MenuScene.hpp"

#include "DebugGlobals.hpp"

#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/gl/Context.hpp>

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

MenuScene::MenuScene(SmashApp& smashApp)
    : Scene(1.0 / 60.0), mSmashApp(smashApp)
{
    mMainWidget.func = [this]() { impl_show_main_window(); };
}

MenuScene::~MenuScene() = default;

//============================================================================//

void MenuScene::refresh_options()
{
}

//============================================================================//

void MenuScene::handle_event(sq::Event event)
{
}

//============================================================================//

void MenuScene::update()
{
}

//============================================================================//

void MenuScene::render(double elapsed)
{
    using Context = sq::Context;

    Context& context = Context::get();

    context.set_ViewPort(mSmashApp.get_options().Window_Size);

    context.clear_Colour({0.f, 0.f, 0.f, 1.f});
    context.clear_Depth_Stencil();
}

//============================================================================//

void MenuScene::impl_show_main_window()
{
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowSizeConstraints({400, 0}, {400, -40});
    ImGui::SetNextWindowPos({20, 20});

    if (ImGui::Begin("Welcome to SuperTuxSmash", nullptr, flags))
    {
        //--------------------------------------------------------//

        ImGui::ComboEnum("Stage", setup.stage);

        //--------------------------------------------------------//

        for (uint index = 0u; index < 4u; ++index)
        {
            auto& player = setup.players[index];

            const String label = "Player %d"_fmt_(index+1);
            if (ImGui::CollapsingHeader(label.c_str()))
            {
                const String label = "Fighter##%d"_fmt_(index);
                ImGui::ComboEnum(label.c_str(), player.fighter, 0);

                player.enabled = (player.fighter != FighterEnum::Null);
            }
        }

        //--------------------------------------------------------//

        if (ImGui::Button("Start Game"))
        {
            const auto predicate = [](const auto& player) { return player.enabled; };
            const bool any = std::any_of(setup.players.begin(), setup.players.end(), predicate);

            mSmashApp.start_game(any ? setup : GameSetup::get_defaults());
        }

        ImGui::SameLine();

        if (ImGui::Button("Start Action Editor"))
        {
            mSmashApp.start_action_editor();
        }
    }

    ImGui::End();
}
