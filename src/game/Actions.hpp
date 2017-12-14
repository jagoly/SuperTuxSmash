#pragma once

#include <functional>

#include <sqee/macros.hpp>
#include <sqee/misc/PoolTools.hpp>

#include "game/FightWorld.hpp"

namespace sts {

//============================================================================//

class Action : private sq::NonCopyable
{
public: //====================================================//

    enum class Type
    {
        None = -1,
        Neutral_First,
        Tilt_Down,
        Tilt_Forward,
        Tilt_Up,
        Air_Back,
        Air_Down,
        Air_Forward,
        Air_Neutral,
        Air_Up,
        Dash_Attack,
        Smash_Down,
        Smash_Forward,
        Smash_Up,
        Special_Down,
        Special_Forward,
        Special_Neutral,
        Special_Up
    };

    //--------------------------------------------------------//

    Action(FightWorld& world, Fighter& fighter, Type type);

    virtual ~Action();

    //--------------------------------------------------------//

    Type get_type() const { return type; }

    //--------------------------------------------------------//

    void do_start();

    bool do_tick();

protected: //=================================================//

    bool finished = false;

    FightWorld& world;
    Fighter& fighter;

    const Type type;
    const string path;

    //--------------------------------------------------------//

    struct Command
    {
        string source;
        std::function<void(Action& action)> func;
    };

    struct TimelineFrame
    {
        uint frame;
        std::vector<Command> commands;
    };

    std::vector<TimelineFrame> timeline;

    sq::TinyPoolMap<sq::TinyString<15>, HitBlob> blobs;

private: //===================================================//

    uint mCurrentFrame = 0u;

    std::vector<TimelineFrame>::iterator mTimelineIter;

    //--------------------------------------------------------//

    friend class Fighter;
    friend class Actions;

    friend struct ActionBuilder;
    friend struct ActionFuncs;
};

//============================================================================//

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(Action::Type)

SQEE_ENUM_TO_STRING_CASE(None)
SQEE_ENUM_TO_STRING_CASE(Neutral_First)
SQEE_ENUM_TO_STRING_CASE(Tilt_Down)
SQEE_ENUM_TO_STRING_CASE(Tilt_Forward)
SQEE_ENUM_TO_STRING_CASE(Tilt_Up)
SQEE_ENUM_TO_STRING_CASE(Air_Back)
SQEE_ENUM_TO_STRING_CASE(Air_Down)
SQEE_ENUM_TO_STRING_CASE(Air_Forward)
SQEE_ENUM_TO_STRING_CASE(Air_Neutral)
SQEE_ENUM_TO_STRING_CASE(Air_Up)
SQEE_ENUM_TO_STRING_CASE(Dash_Attack)
SQEE_ENUM_TO_STRING_CASE(Smash_Down)
SQEE_ENUM_TO_STRING_CASE(Smash_Forward)
SQEE_ENUM_TO_STRING_CASE(Smash_Up)
SQEE_ENUM_TO_STRING_CASE(Special_Down)
SQEE_ENUM_TO_STRING_CASE(Special_Forward)
SQEE_ENUM_TO_STRING_CASE(Special_Neutral)
SQEE_ENUM_TO_STRING_CASE(Special_Up)

SQEE_ENUM_TO_STRING_BLOCK_END

//============================================================================//

} // namespace sts
