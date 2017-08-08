#include "stages/TestZone_Stage.hpp"

#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"

#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"

#include "game/GameScene.hpp"

using namespace sts;

//============================================================================//

GameScene::GameScene(const sq::InputDevices& inputDevices, const Options& options)
    : Scene(1.0 / 12.0), mInputDevices(inputDevices), mOptions(options)
{
    mRenderer = std::make_unique<Renderer>(options);

    mFightWorld = std::make_unique<FightWorld>();

    mStage = std::make_unique<TestZone_Stage>();

    mControllers[0] = std::make_unique<Controller>(0u, mInputDevices, "player1.json");
    mControllers[1] = std::make_unique<Controller>(1u, mInputDevices, "player2.json");

    auto fighterA = std::make_unique<Sara_Fighter>(0u, *mFightWorld, *mControllers[0]);
    auto fighterB = std::make_unique<Tux_Fighter>(1u, *mFightWorld, *mControllers[1]);

    auto renderFighterA = std::make_unique<Sara_Render>(*mRenderer, *fighterA);
    auto renderFighterB = std::make_unique<Tux_Render>(*mRenderer, *fighterB);

    mFightWorld->add_fighter(std::move(fighterA));
    mFightWorld->add_fighter(std::move(fighterB));

    mRenderer->add_object(std::move(renderFighterA));
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
    mStage->tick();

    mFightWorld->tick();

    //--------------------------------------------------------//

    const auto viewBounds = mFightWorld->compute_camera_view_bounds();
    mRenderer->set_camera_view_bounds(viewBounds.first, viewBounds.second);
}

//============================================================================//

void GameScene::render(double elapsed)
{
    mRenderer->render_objects(float(elapsed), float(mAccumulation / mTickTime));

    mRenderer->render_blobs(mFightWorld->get_hit_blobs());
    mRenderer->render_blobs(mFightWorld->get_hurt_blobs());

    mRenderer->finish_rendering();
}
