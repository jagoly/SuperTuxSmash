#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>

#include "DebugGlobals.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"

#include "game/ActionBuilder.hpp"

#include "game/GameScene.hpp"

namespace gui = sq::gui;
using namespace sts;

//============================================================================//

GameScene::GameScene(const sq::InputDevices& inputDevices, const Options& options)
    : Scene(1.0 / 48.0), mInputDevices(inputDevices), mOptions(options)
{
    mGeneralWidget.func = [this]() { impl_show_general_window(); };
    mFightersWidget.func = [this]() { impl_show_fighters_window(); };

    //mGeneralWidget.add_to_system(sq::GuiSystem::get());
    //mFightersWidget.add_to_system(sq::GuiSystem::get());

    //--------------------------------------------------------//

    mFightWorld = std::make_unique<FightWorld>();

    mRenderer = std::make_unique<Renderer>(options);

    //--------------------------------------------------------//

    mControllers[0] = std::make_unique<Controller>(mInputDevices, "player1.json");
    mControllers[1] = std::make_unique<Controller>(mInputDevices, "player2.json");

    auto stage = std::make_unique<TestZone_Stage>(*mFightWorld);
    auto renderStage = std::make_unique<TestZone_Render>(*mRenderer, *stage);
    mFightWorld->set_stage(std::move(stage));
    mRenderer->add_object(std::move(renderStage));

    auto fighterA = std::make_unique<Sara_Fighter>(0u, *mFightWorld);
    fighterA->set_controller(mControllers[0].get());
    auto renderFighterA = std::make_unique<Sara_Render>(*mRenderer, *fighterA);
    mFightWorld->add_fighter(std::move(fighterA));
    mRenderer->add_object(std::move(renderFighterA));

    auto fighterB = std::make_unique<Tux_Fighter>(1u, *mFightWorld);
    fighterB->set_controller(mControllers[1].get());
    auto renderFighterB = std::make_unique<Tux_Render>(*mRenderer, *fighterB);
    mFightWorld->add_fighter(std::move(fighterB));
    mRenderer->add_object(std::move(renderFighterB));
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
        if (controller != nullptr)
            controller->handle_event(event);
    }
}

//============================================================================//

void GameScene::update()
{
    mFightWorld->tick();

    mRenderer->update_from_scene_data(mFightWorld->compute_scene_data());
}

//============================================================================//

void GameScene::render(double elapsed)
{
    const float accum = float(elapsed);
    const float blend = float(mAccumulation / mTickTime);

    mRenderer->render_objects(accum, blend);
    mRenderer->render_particles(accum, blend);

    if (dbg.renderBlobs == true)
    {
        mRenderer->render_blobs(mFightWorld->get_hit_blobs());
        mRenderer->render_blobs(mFightWorld->get_hurt_blobs());
    }

    mRenderer->finish_rendering();
}

//============================================================================//

void GameScene::impl_show_general_window()
{
    constexpr auto flags = gui::Window::AutoSize | gui::Window::NoMove;
    const auto window = gui::scope_window("General Debug", {300, 0}, {300, 200}, {+20, +20}, flags);

    if (window.want_display() == false) return;

    //--------------------------------------------------------//

    WITH (gui::scope_framed_group())
    {
        if (gui::button_with_tooltip("reload actions", "reload actions from json"))
        {
            for (Fighter* fighter : mFightWorld->get_fighters())
                for (int t = 0; t < 14; ++t)
                    ActionBuilder::load_from_json(*fighter->get_actions()[Action::Type(t)]);

            if (auto fighters = mFightWorld->get_fighters(); fighters.size() >= 2u)
            {
                auto controllerLast = fighters.back()->get_controller();
                if (fighters.size() == 4u) fighters[3]->set_controller(fighters[2]->get_controller());
                if (fighters.size() >= 3u) fighters[2]->set_controller(fighters[1]->get_controller());
                fighters[1]->set_controller(fighters[0]->get_controller());
                fighters[0]->set_controller(controllerLast);
            }
        }

        imgui::SameLine();

        if (gui::button_with_tooltip("swap control", "cycle the controllers"))
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

        imgui::Checkbox("disable input", &dbg.disableInput);
        imgui::SameLine();
        imgui::Checkbox("render blobs", &dbg.renderBlobs);
    }
}

//============================================================================//

void GameScene::impl_show_fighters_window()
{
    constexpr auto flags = gui::Window::AutoSize | gui::Window::NoMove;
    const auto window = gui::scope_window("Fighter Debug", {380, 0}, {380, -40}, {-20, +20}, flags);

    if (window.want_display() == false) return;

    //--------------------------------------------------------//

    WITH (gui::scope_item_width(-FLT_EPSILON))
    {
        for (Fighter* fighter : mFightWorld->get_fighters())
            fighter->impl_show_fighter_widget();
    }
}
