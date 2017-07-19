#include "stages/TestZone_Stage.hpp"

#include "fighters/Sara_Fighter.hpp"
#include "fighters/Cheese_Fighter.hpp"

#include "fighters/Sara_Render.hpp"
#include "fighters/Cheese_Render.hpp"

#include "game/GameScene.hpp"

using namespace sts;

//============================================================================//

GameScene::GameScene(const sq::InputDevices& inputDevices, const Options& options)
    : Scene(1.0 / 48.0), mInputDevices(inputDevices), mOptions(options)
{
    mRenderer = std::make_unique<Renderer>(options);

    mFightSystem = std::make_unique<FightSystem>();

    mStage = std::make_unique<TestZone_Stage>();

    mControllers[0] = std::make_unique<Controller>(0u, mInputDevices, "player1.txt");
    mControllers[1] = std::make_unique<Controller>(1u, mInputDevices, "player2.txt");

    mFighters[0] = std::make_unique<Sara_Fighter>(0u, *mFightSystem, *mControllers[0]);
    mFighters[1] = std::make_unique<Cheese_Fighter>(1u, *mFightSystem, *mControllers[1]);

    mRenderer->add_entity(std::make_unique<Sara_Render>(*mFighters[0], *mRenderer));
    mRenderer->add_entity(std::make_unique<Cheese_Render>(*mFighters[1], *mRenderer));
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

    //--------------------------------------------------------//

    for (auto& fighter : mFighters)
    {
        if (fighter != nullptr)
            fighter->tick();
    }

    //--------------------------------------------------------//

    mFightSystem->tick();
}

//============================================================================//

void GameScene::render(double elapsed)
{
    mRenderer->render(float(elapsed), float(mAccumulation / mTickTime));

    mRenderer->render_hit_blobs(mFightSystem->get_offensive_blobs());
    mRenderer->render_hit_blobs(mFightSystem->get_damageable_blobs());
}
