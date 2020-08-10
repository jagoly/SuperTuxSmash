#pragma once

#include "setup.hpp"

#include "game/ActionEnums.hpp"

#include <sqee/redist/sol.hpp>

namespace sts {

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

    void do_start();

    void do_tick();

    void do_cancel();

    //--------------------------------------------------------//

    ActionStatus get_status() { return mStatus; };

    //--------------------------------------------------------//

    void load_from_json();

    void load_lua_from_fallback();

    void load_lua_from_file();

    void load_lua_from_string();

    //--------------------------------------------------------//

    void lua_func_wait_until(uint frame);

    //void lua_func_enable_blob(TinyString key);

    //void lua_func_disable_blob(TinyString key);

    void lua_func_enable_blob_group(uint8_t group);

    void lua_func_disable_blob_group(uint8_t group);

    void lua_func_emit_particles(TinyString key, uint count);

    void lua_func_allow_interrupt();

private: //===================================================//

    ActionStatus mStatus = ActionStatus::None;

    sq::PoolMap<TinyString, HitBlob> mBlobs;

    sq::PoolMap<TinyString, Emitter> mEmitters;

    //--------------------------------------------------------//

    String mLuaSource;
    String mErrorMessage;

    //--------------------------------------------------------//

    uint mCurrentFrame = 0u;
    uint mWaitingUntil = 0u;

    bool mAllowIterrupt = false;

    //--------------------------------------------------------//

    sol::environment mEnvironment;

    sol::function mTickFunction;
    sol::coroutine mTickCoroutine;

    //sol::function mCancelFunction;

    sol::thread mThread;

    //--------------------------------------------------------//

    void reset_lua_thread();
    void reset_lua_environment();

    //--------------------------------------------------------//

    bool has_changes(const Action& reference) const;

    void apply_changes(const Action& source);

    std::unique_ptr<Action> clone() const;

    //--------------------------------------------------------//

    template <class... Args>
    inline void log_script(StringView str, const Args&... args);

    //--------------------------------------------------------//

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts
