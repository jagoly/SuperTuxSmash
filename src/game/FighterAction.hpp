#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct FighterActionDef final
{
    FighterActionDef(const FighterDef& fighter, SmallString name);

    ~FighterActionDef();

    //--------------------------------------------------------//

    const FighterDef& fighter;

    const SmallString name;

    std::map<TinyString, HitBlobDef> blobs;
    std::map<TinyString, VisualEffectDef> effects;
    std::map<TinyString, Emitter> emitters;

    WrenHandle* scriptClass = nullptr;

    // todo: find a way to move this to the editor
    String wrenSource;

    //--------------------------------------------------------//

    void load_json_from_file();

    void load_wren_from_file();

    void interpret_module();
};

//============================================================================//

class FighterAction final : sq::NonCopyable
{
public: //====================================================//

    FighterAction(const FighterActionDef& def, Fighter& fighter);

    ~FighterAction();

    void initialise_script();

    //--------------------------------------------------------//

    const FighterActionDef& def;

    Fighter& fighter;

    World& world;

    //--------------------------------------------------------//

    void call_do_start();

    void call_do_updates();

    void call_do_cancel();

    //-- wren methods ----------------------------------------//

    const SmallString& wren_get_name() { return def.name; }

    Fighter* wren_get_fighter() { return &fighter; }

    World* wren_get_world() { return &world; }

    WrenHandle* wren_get_script() { return mScriptHandle; }

    WrenHandle* wren_get_fiber() { return mFiberHandle; }

    void wren_set_fiber(WrenHandle* fiber) { mFiberHandle = fiber; }

    void wren_log_with_prefix(StringView message);

    void wren_cxx_before_start();

    void wren_cxx_wait_until(uint frame);

    void wren_cxx_wait_for(uint frames);

    bool wren_cxx_next_frame();

    void wren_cxx_before_cancel();

    void wren_enable_hitblobs(StringView prefix);

    void wren_disable_hitblobs(bool resetCollisions);

    int32_t wren_play_effect(TinyString key);

    void wren_emit_particles(TinyString key);

private: //===================================================//

    // todo: only mScriptHandle is needed per action, move the rest to Fighter

    uint mCurrentFrame = 0u;
    uint mWaitUntil = 0u;

    WrenHandle* mScriptHandle = nullptr;
    WrenHandle* mFiberHandle = nullptr;

    //--------------------------------------------------------//

    void set_error_message(StringView method, StringView error);

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::FighterAction)
