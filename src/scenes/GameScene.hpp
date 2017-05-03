#pragma once

#include <sqee/builtins.hpp>
#include <sqee/app/Scene.hpp>

namespace sts {

//============================================================================//

class SmashApp; // Forward Declaration
class Game; // Forward Declaration

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

    unique_ptr<Game> mGame;
};

//============================================================================//

} // namespace sts
