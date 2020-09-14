#pragma once

#include "setup.hpp"

#include "game/ActionEnums.hpp"

#include <sqee/app/WrenPlus.hpp>

namespace sts {

//============================================================================//

class Action final : sq::NonCopyable
{
public: //====================================================//

    Fighter& fighter;
    FightWorld& world;

    const ActionType type;

    //--------------------------------------------------------//

    Action(Fighter& fighter, ActionType type);

    ~Action();

    //--------------------------------------------------------//

    void do_start();

    void do_tick();

    void do_cancel();

    //--------------------------------------------------------//

    ActionStatus get_status() const { return mStatus; };

    uint get_current_frame() const { return mCurrentFrame; }

    //--------------------------------------------------------//

    void set_flag(ActionFlag flag, bool value) { if (value) mFlags |= uint8_t(flag); else mFlags &= ~uint8_t(flag); }

    bool check_flag(ActionFlag flag) const { return (uint8_t(flag) & mFlags) != 0u; }

    //--------------------------------------------------------//

    void load_json_from_file();

    void load_wren_from_file();

    void load_wren_from_string();

    //-- wren methods and properties -------------------------//

    void wren_set_wait_until(uint frame);

    void wren_allow_interrupt();

    void wren_enable_hitblob_group(uint8_t group);

    void wren_disable_hitblob_group(uint8_t group);

    void wren_enable_hitblob(TinyString key);

    void wren_disable_hitblob(TinyString key);

    void wren_disable_hitblobs();

    void wren_emit_particles(TinyString key);

    void wren_play_sound(TinyString key);

    void wren_cancel_sound(TinyString key);

    void wren_set_flag_AllowNext();
    void wren_set_flag_AutoJab();

    bool wren_check_flag_AttackHeld() const;
    bool wren_check_flag_HitCollide() const;

private: //===================================================//

    ActionStatus mStatus = ActionStatus::None;

    uint8_t mFlags = 0u;

    uint mCurrentFrame = 0u;
    uint mWaitingUntil = 0u;

    String mWrenSource;
    String mErrorMessage;

    WrenHandle* mScriptInstance = nullptr;
    WrenHandle* mFiberInstance = nullptr;

    //--------------------------------------------------------//

    std::pmr::map<TinyString, HitBlob> mBlobs;

    std::pmr::map<TinyString, Emitter> mEmitters;

    std::pmr::map<TinyString, SoundEffect> mSounds;

    //--------------------------------------------------------//

    String build_path(StringView extension) const;

    bool has_changes(const Action& reference) const;

    void apply_changes(const Action& source);

    std::unique_ptr<Action> clone() const;

    //--------------------------------------------------------//

    template <class... Args>
    void impl_set_error_message(StringView str, const Args&... args);

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
