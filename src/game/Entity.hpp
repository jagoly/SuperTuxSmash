#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include "game/forward.hpp"

//============================================================================//

namespace sts {

class Entity : sq::NonCopyable
{
public: //====================================================//

    Entity(FightSystem& system);

    virtual ~Entity() = default;

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    Vec2F mPreviousPosition;
    Vec2F mCurrentPosition;

protected: //=================================================//

    void base_tick_entity();

    //--------------------------------------------------------//

    FightSystem& mFightSystem;
};

} // namespace sts
