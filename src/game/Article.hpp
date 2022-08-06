#pragma once

#include "setup.hpp"

#include "game/ArticleDef.hpp"
#include "game/Entity.hpp"

namespace sts {

//============================================================================//

class Article final : public Entity
{
public: //====================================================//

    /// Public information and variables for articles.
    struct Variables : EntityVars
    {
        bool fragile = false;
        bool bounced = false;
    };

    //--------------------------------------------------------//

    Article(const ArticleDef& def, Fighter* fighter);

    ~Article();

    //--------------------------------------------------------//

    const ArticleDef& def;

    Fighter* const fighter;

    Variables variables;

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    void call_do_updates();

    void call_do_destroy();

    //--------------------------------------------------------//

    bool check_marked_for_destroy() const { return mMarkedForDestroy; }

    const EntityDef& get_def() const override { return def; }

    EntityVars& get_vars() override { return variables; }

    const EntityVars& get_vars() const override { return variables; }

    //-- wren methods ----------------------------------------//

    WrenHandle* wren_get_script_class() { return def.scriptClass; }

    WrenHandle* wren_get_script() { return mScriptHandle; }

    void wren_set_script(WrenHandle* fiber) { mScriptHandle = fiber; }

    WrenHandle* wren_get_fiber() { return mFiberHandle; }

    void wren_set_fiber(WrenHandle* fiber) { mFiberHandle = fiber; }

    void wren_log_with_prefix(StringView message);

    void wren_cxx_wait_until(uint frame);

    void wren_cxx_wait_for(uint frames);

    bool wren_cxx_next_frame();

    void wren_mark_for_destroy() { mMarkedForDestroy = true; }

    void wren_enable_hitblobs(StringView prefix);

    void wren_disable_hitblobs(bool resetCollisions);

    int32_t wren_play_effect(TinyString key);

    void wren_emit_particles(TinyString key);

private: //===================================================//

    bool mJustCreated = true;
    bool mMarkedForDestroy = false;

    uint mCurrentFrame = 0u;
    uint mWaitUntil = 0u;

    WrenHandle* mScriptHandle = nullptr;
    WrenHandle* mFiberHandle = nullptr;

    void set_error_message(StringView method, StringView error);
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::Article)
WRENPLUS_TRAITS_HEADER(sts::Article::Variables)
