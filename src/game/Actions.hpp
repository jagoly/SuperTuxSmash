#pragma once

#include <functional>

#include <sqee/macros.hpp>
#include <sqee/misc/PoolTools.hpp>
#include <sqee/misc/StaticVector.hpp>

#include "game/FightWorld.hpp"
#include "game/ParticleEmitter.hpp"

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
        Special_Up,
        Count
    };

    //--------------------------------------------------------//

    Action(FightWorld& world, Fighter& fighter, Type type);

    virtual ~Action();

    //--------------------------------------------------------//

    Type get_type() const { return type; }

    //--------------------------------------------------------//

    void do_start();

    bool do_tick();

    void do_finish();

protected: //=================================================//

    bool finished = false;

    FightWorld& world;
    Fighter& fighter;

    const Type type;
    const String path;

    //--------------------------------------------------------//

    struct Command
    {
        Vector<uint16_t> frames;
        std::function<void(Action& action)> func;
        String source;
    };

    Vector<Command> commands;

    sq::TinyPoolMap<TinyString, HitBlob> blobs;

    sq::TinyPoolMap<TinyString, ParticleEmitter> emitters;

private: //===================================================//

    uint16_t mCurrentFrame = 0u;

    Vector<Command>::iterator mCommandIter;

    //--------------------------------------------------------//

    friend class Fighter;
    friend class Actions;

    friend struct ActionBuilder;
    friend struct ActionFuncs;
};

//============================================================================//

SQEE_ENUM_TO_STRING(Action::Type, None, Neutral_First, Tilt_Down, Tilt_Forward, Tilt_Up, Air_Back, Air_Down,
                    Air_Forward, Air_Neutral, Air_Up, Dash_Attack, Smash_Down, Smash_Forward, Smash_Up, Special_Down,
                    Special_Forward, Special_Neutral, Special_Up, Count)

SQEE_ENUM_TO_STRING_STREAM_OPERATOR(Action::Type)

//============================================================================//

} // namespace sts
