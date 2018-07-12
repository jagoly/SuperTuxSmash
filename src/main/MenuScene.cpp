#include <sqee/macros.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/gl/Context.hpp>

#include "DebugGlobals.hpp"

#include "main/MenuScene.hpp"

namespace gui = sq::gui;
using sq::literals::operator""_fmt_;
using namespace sts;

//============================================================================//

MenuScene::MenuScene(SmashApp& smashApp) : Scene(1.0 / 60.0), mSmashApp(smashApp)
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
    string strBuf; strBuf.reserve(50);

    if (!gui::begin_window("Welcome to SuperTuxSmash", {400, 0}, {400, -40}, {20.f, 20.f})) return;

    //--------------------------------------------------------//

    {
        int8_t& ref = reinterpret_cast<int8_t&>(setup.stage);
        const auto getter = [](int8_t i) { return enum_to_string(StageEnum(i)); };

        gui::input_combo("Stage", 100.f, ref, getter, int8_t(StageEnum::Count));
    }

    //--------------------------------------------------------//

    for (uint8_t index = 0u; index < 4u; ++index)
    {
        setup.players[index].enabled = gui::begin_collapse("Player %d"_fmt_(index+1));

        if (setup.players[index].enabled == true)
        {
            int8_t& ref = reinterpret_cast<int8_t&>(setup.players[index].fighter);
            const auto getter = [](int8_t i) { return enum_to_string(FighterEnum(i)); };

            gui::input_combo("Fighter", 100.f, ref, getter, int8_t(FighterEnum::Count));

            gui::end_collapse();
        }
    }

    //--------------------------------------------------------//

    if (imgui::Button("Start Game") == true)
    {
        bool any = false;

        for (uint8_t index = 0u; index < 4u; ++index)
            any |= setup.players[index].enabled;

        mSmashApp.start_game(any ? setup : GameSetup::get_defaults());
    }

    //--------------------------------------------------------//

    gui::end_window();
}
