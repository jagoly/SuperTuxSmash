#pragma once

#include <sqee/builtins.hpp>
#include <sqee/app/Scene.hpp>

namespace sts {

//============================================================================//

// Forward Declarations /////

class SmashApp;
class Renderer;

class Stage;
class Controller;
class Fighter;

//============================================================================//

class GameScene : public sq::Scene
{
public:

    //========================================================//

    GameScene(SmashApp& smashApp);

    //========================================================//

    void update_options();
    bool handle(sf::Event event);
    void tick(); void render();

private:

    //========================================================//

    SmashApp& mSmashApp;

    unique_ptr<Renderer> mRenderer;

    unique_ptr<Stage> mStage;

    vector<unique_ptr<Controller>> mControllers;

    vector<unique_ptr<Fighter>> mFighters;
};

//============================================================================//

} // namespace sts
