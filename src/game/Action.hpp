#pragma once

#include "setup.hpp"

#include "game/ActionEnums.hpp"

#include <sqee/app/WrenPlus.hpp>

namespace sts {

//============================================================================//

class Action : private sq::NonCopyable
{
public: //====================================================//

    const ActionType type;

    Fighter& fighter;
    FightWorld& world;

    const String path;

    //--------------------------------------------------------//

    Action(FightWorld& world, Fighter& fighter, ActionType type, String path);

    virtual ~Action();

    //--------------------------------------------------------//

    void do_start();

    void do_tick();
    //void do_tick(const InputFrame& input);

    void do_cancel();

    //--------------------------------------------------------//

    ActionStatus get_status() const { return mStatus; };

    uint get_current_frame() const { return mCurrentFrame; }

    //--------------------------------------------------------//

    void load_from_json();

    void load_wren_from_file();

    void load_wren_from_string();

    //-- wren methods and properties -------------------------//

    void wren_set_wait_until(uint frame);

    void wren_allow_interrupt();

    void wren_enable_hitblob_group(uint8_t group);

    void wren_disable_hitblob_group(uint8_t group);

    void wren_disable_hitblobs();

    void wren_emit_particles(TinyString key);

    void wren_play_sound(TinyString key);

private: //===================================================//

    ActionStatus mStatus = ActionStatus::None;

    uint mCurrentFrame = 0u;
    uint mWaitingUntil = 0u;

    String mWrenSource;
    String mErrorMessage;

    WrenHandle* mScriptInstance = nullptr;
    WrenHandle* mFiberInstance = nullptr;

    //--------------------------------------------------------//

    sq::PoolMap<TinyString, HitBlob> mBlobs;

    sq::PoolMap<TinyString, Emitter> mEmitters;

    sq::PoolMap<TinyString, SoundEffect> mSounds;

    //--------------------------------------------------------//

    bool has_changes(const Action& reference) const;

    void apply_changes(const Action& source);

    std::unique_ptr<Action> clone() const;

    //--------------------------------------------------------//

    template <class... Args>
    void set_error_status(StringView str, const Args&... args);

    //--------------------------------------------------------//

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

template<> struct wren::Traits<sts::Action> : std::true_type
{
    static constexpr const char module[] = "sts";
    static constexpr const char className[] = "Action";
};
