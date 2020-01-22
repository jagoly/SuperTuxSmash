#pragma once

#include <functional>
#include <map>

#include <sqee/macros.hpp>
#include <sqee/misc/PoolTools.hpp>
#include <sqee/misc/TinyString.hpp>

#include "game/Blobs.hpp"
#include "game/ParticleEmitter.hpp"

namespace sts {

//============================================================================//

enum class ActionType : int8_t
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

//============================================================================//

class Action : private sq::NonCopyable
{
public: //====================================================//

    using Command = std::function<void(Action& action)>;

    struct Procedure
    {
        Vector<Command> commands;

        struct Meta {
            String source;
            Vector<uint16_t> frames;
        } meta;

        bool operator==(const Procedure& other) const
        {
            return meta.source == other.meta.source && meta.frames == other.meta.frames;
        }
    };

    //--------------------------------------------------------//

    Action(FightWorld& world, Fighter& fighter, ActionType type, String path);

    virtual ~Action();

    //--------------------------------------------------------//

    ActionType get_type() const { return type; }

    const Fighter& get_fighter() const { return fighter; }

    //--------------------------------------------------------//

    void do_start();

    bool do_tick();

    void do_finish();

    //--------------------------------------------------------//

    void rebuild_timeline();

protected: //=================================================//

    bool finished = false;

    FightWorld& world;
    Fighter& fighter;

    const ActionType type;
    const String path;

    //--------------------------------------------------------//

    sq::PoolMap<TinyString, HitBlob> blobs;

    sq::PoolMap<TinyString, ParticleEmitter> emitters;

    std::map<TinyString, Procedure> procedures;

    //--------------------------------------------------------//

    struct TimelineItem
    {
        Vector<std::reference_wrapper<const Procedure>> procedures;
    };

    Vector<TimelineItem> timeline;

private: //===================================================//

    uint16_t mCurrentFrame = 0u;

    //--------------------------------------------------------//

    // for the action editor

    bool has_changes(const Action& reference) const;

    void apply_changes(const Action& source);

    UniquePtr<Action> clone() const;

    //--------------------------------------------------------//

    friend class Fighter;
    friend class Actions;

    friend class ActionBuilder;
    friend class EditorScene;

    friend struct ActionFuncs;
    friend struct ActionFuncsValidate;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::ActionType, None, Neutral_First, Tilt_Down, Tilt_Forward, Tilt_Up, Air_Back, Air_Down,
                 Air_Forward, Air_Neutral, Air_Up, Dash_Attack, Smash_Down, Smash_Forward, Smash_Up, Special_Down,
                 Special_Forward, Special_Neutral, Special_Up)
