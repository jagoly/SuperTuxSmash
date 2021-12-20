#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

class FighterAction final : sq::NonCopyable
{
public: //====================================================//

    FighterAction(Fighter& fighter, SmallString name);

    ~FighterAction();

    //--------------------------------------------------------//

    Fighter& fighter;
    FightWorld& world;

    const SmallString name;

    //--------------------------------------------------------//

    void call_do_start();

    void call_do_updates();

    void call_do_cancel();

    //--------------------------------------------------------//

    void set_hit_something() { mHitSomething = true; }

    //--------------------------------------------------------//

    void load_json_from_file();

    void load_wren_from_file();

    void load_wren_from_string();

    //-- wren methods ----------------------------------------//

    Fighter* wren_get_fighter() { return &fighter; }

    WrenHandle* wren_get_script() { return mScriptHandle; }

    WrenHandle* wren_get_fiber() { return mFiberHandle; }

    void wren_set_fiber(WrenHandle* fiber) { mFiberHandle = fiber; }

    void wren_log_with_prefix(StringView message);

    void wren_cxx_before_start();

    void wren_cxx_wait_until(uint frame);

    void wren_cxx_wait_for(uint frames);

    bool wren_cxx_next_frame();

    void wren_cxx_before_cancel();

    bool wren_check_hit_something() { return mHitSomething; }

    void wren_enable_hitblobs(StringView prefix);

    void wren_disable_hitblobs(bool resetCollisions);

    void wren_play_effect(TinyString key);

    void wren_emit_particles(TinyString key);

    void wren_play_sound(TinyString key);

    void wren_cancel_sound(TinyString key);

private: //===================================================//

    uint mCurrentFrame = 0u;
    uint mWaitUntil = 0u;

    WrenHandle* mScriptHandle = nullptr;
    WrenHandle* mFiberHandle = nullptr;

    // todo: should be a list of things
    bool mHitSomething = false;

    std::map<TinyString, HitBlob> mBlobs;
    std::map<TinyString, VisualEffect> mEffects;
    std::map<TinyString, Emitter> mEmitters;
    std::map<TinyString, SoundEffect> mSounds;

    // todo: find a way to move this to the editor
    String mWrenSource;

    //--------------------------------------------------------//

    bool has_changes(const FighterAction& reference) const;

    void apply_changes(const FighterAction& source);

    std::unique_ptr<FighterAction> clone() const;

    void set_error_message(StringView method, StringView error);

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::FighterAction)
