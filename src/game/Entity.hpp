#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include "game/Game.hpp"

//============================================================================//

namespace sts {

class Entity : sq::NonCopyable
{
public: //====================================================//

    Entity(string name, Game& game);

    virtual ~Entity() = default;

    //--------------------------------------------------------//

    const string name;
    Game& game;

    //--------------------------------------------------------//

    virtual void setup() = 0;

    virtual void tick() = 0;

    virtual void integrate(float blend) = 0;

    virtual void render_depth() = 0;

    virtual void render_main() = 0;

    //--------------------------------------------------------//

    Vec2F mPreviousPosition;
    Vec2F mCurrentPosition;

protected: //=================================================//

    void base_tick_Entity();
};

} // namespace sts
