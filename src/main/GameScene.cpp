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

#include "main/GameScene.hpp"

namespace gui = sq::gui;
using namespace sts;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp, GameSetup setup) : Scene(1.0 / 48.0)
  , mInputDevices(smashApp.get_input_devices()), mOptions(smashApp.get_options())
{
    mGeneralWidget.func = [this]() { impl_show_general_window(); };
    mFightersWidget.func = [this]() { impl_show_fighters_window(); };

    //--------------------------------------------------------//

    mFightWorld = std::make_unique<FightWorld>();
    mRenderer = std::make_unique<Renderer>(mOptions);

    //--------------------------------------------------------//

    mControllers[0] = std::make_unique<Controller>(mInputDevices, "player1.json");
    mControllers[1] = std::make_unique<Controller>(mInputDevices, "player2.json");
    mControllers[2] = std::make_unique<Controller>(mInputDevices, "player3.json");
    mControllers[3] = std::make_unique<Controller>(mInputDevices, "player4.json");

    //--------------------------------------------------------//

    unique_ptr<Stage> stage;
    unique_ptr<RenderObject> renderStage;

    SWITCH (setup.stage)
    {
        CASE (TestZone)
        stage = std::make_unique<TestZone_Stage>(*mFightWorld);
        renderStage = std::make_unique<TestZone_Render>(*mRenderer, static_cast<TestZone_Stage&>(*stage));

        CASE_DEFAULT SQASSERT(false, "bad stage setup");
    }
    SWITCH_END;

    mFightWorld->set_stage(std::move(stage));
    mRenderer->add_object(std::move(renderStage));

    //--------------------------------------------------------//

    for (uint8_t index = 0u; index < 4u; ++index)
    {
        if (setup.players[index].enabled == false) continue;

        unique_ptr<Fighter> fighter;
        unique_ptr<RenderObject> renderFighter;

        SWITCH (setup.players[index].fighter)
        {
            CASE (Sara)
            fighter = std::make_unique<Sara_Fighter>(index, *mFightWorld);
            renderFighter = std::make_unique<Sara_Render>(*mRenderer, static_cast<Sara_Fighter&>(*fighter));

            CASE (Tux)
            fighter = std::make_unique<Tux_Fighter>(index, *mFightWorld);
            renderFighter = std::make_unique<Tux_Render>(*mRenderer, static_cast<Tux_Fighter&>(*fighter));

            CASE_DEFAULT SQASSERT(false, "bad fighter setup");
        }
        SWITCH_END;

        fighter->set_controller(mControllers[index].get());

        mFightWorld->add_fighter(std::move(fighter));
        mRenderer->add_object(std::move(renderFighter));
    }
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
    mRenderer->render_particles(mFightWorld->get_particle_set(), accum, blend);

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
    if (!gui::begin_window("General Debug", {300, 0}, {300, 200}, {+20, +20})) return;

    //--------------------------------------------------------//

    if (gui::button_with_tooltip("reload actions", "reload actions from json"))
    {
        for (Fighter* fighter : mFightWorld->get_fighters())
            fighter->debug_reload_actions();
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

    //--------------------------------------------------------//

    gui::end_window();
}

//============================================================================//

void GameScene::impl_show_fighters_window()
{
    if (!gui::begin_window("Fighter Debug", {380, 0}, {380, -40}, {-20, +20})) return;

    //--------------------------------------------------------//

    for (Fighter* fighter : mFightWorld->get_fighters())
        fighter->debug_show_fighter_widget();

    //--------------------------------------------------------//

    gui::end_window();
}
