#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

//============================================================================//

namespace sts {

class Entity : sq::NonCopyable
{
public: //====================================================//

    Entity(const string& name);

    virtual ~Entity() = default;

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const string name;

    //--------------------------------------------------------//

    Vec2F mPreviousPosition;
    Vec2F mCurrentPosition;

protected: //=================================================//

    void base_tick_entity();
};

} // namespace sts
