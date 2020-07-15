#pragma once

#include <sqee/redist/sol.hpp>

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
    NeutralFirst,
    TiltDown,
    TiltForward,
    TiltUp,
    AirBack,
    AirDown,
    AirForward,
    AirNeutral,
    AirUp,
    DashAttack,
    SmashDown,
    SmashForward,
    SmashUp,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp,
    EvadeBack,
    EvadeForward,
    Dodge,
    AirDodge
};

//============================================================================//

class Action : private sq::NonCopyable
{
public: //====================================================//

    FightWorld& world;
    Fighter& fighter;

    const ActionType type;
    const String path;

    //--------------------------------------------------------//

    Action(FightWorld& world, Fighter& fighter, ActionType type, String path);

    virtual ~Action();

    //--------------------------------------------------------//

    ActionType get_type() const { return type; }

    const Fighter& get_fighter() const { return fighter; }

    //--------------------------------------------------------//

    void do_start();

    bool do_tick();

    void do_cancel();

    //--------------------------------------------------------//

    void load_from_json();

    void load_lua_from_file();

    void load_lua_from_string(StringView source);

    //--------------------------------------------------------//

    void lua_func_wait_until(uint frame);

    void lua_func_finish_action();

    void lua_func_enable_blob(TinyString key);

    void lua_func_disable_blob(TinyString key);

    void lua_func_emit_particles(TinyString key, uint count);

private: //===================================================//

    sq::PoolMap<TinyString, HitBlob> mBlobs;

    sq::PoolMap<TinyString, ParticleEmitter> mEmitters;

    String mLuaSource; // only used for editor

    //--------------------------------------------------------//

    uint mCurrentFrame = 0u;
    uint mWaitingUntil = 0u;

    bool mFinished = false;

    //--------------------------------------------------------//

    sol::environment mEnvironment;

    sol::function mTickFunction;
    sol::coroutine mTickCoroutine;

    sol::function mCancelFunction;

    sol::thread mThread;

    //--------------------------------------------------------//

    bool has_changes(const Action& reference) const;

    void apply_changes(const Action& source);

    UniquePtr<Action> clone() const;

    //--------------------------------------------------------//

    friend struct DebugGui;
    friend class EditorScene;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::ActionType,
                 None,
                 NeutralFirst,
                 TiltDown,
                 TiltForward,
                 TiltUp,
                 AirBack,
                 AirDown,
                 AirForward,
                 AirNeutral,
                 AirUp,
                 DashAttack,
                 SmashDown,
                 SmashForward,
                 SmashUp,
                 SpecialDown,
                 SpecialForward,
                 SpecialNeutral,
                 SpecialUp,
                 EvadeBack,
                 EvadeForward,
                 Dodge,
                 AirDodge)
