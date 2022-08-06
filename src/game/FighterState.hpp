#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct FighterStateDef final
{
    FighterStateDef(const FighterDef& fighter, TinyString name);

    ~FighterStateDef();

    //--------------------------------------------------------//

    const FighterDef& fighter;

    const TinyString name;

    WrenHandle* scriptClass = nullptr;

    //--------------------------------------------------------//

    void load_wren_from_file();
};

//============================================================================//

class FighterState final : sq::NonCopyable
{
public: //====================================================//

    FighterState(const FighterStateDef& def, Fighter& fighter);

    ~FighterState();

    //--------------------------------------------------------//

    const FighterStateDef& def;

    Fighter& fighter;

    World& world;

    //--------------------------------------------------------//

    void call_do_enter();

    void call_do_updates();

    void call_do_exit();

    //-- wren methods ----------------------------------------//

    const TinyString& wren_get_name() { return def.name; }

    Fighter* wren_get_fighter() { return &fighter; }

    World* wren_get_world() { return &world; }

    WrenHandle* wren_get_script() { return mScriptHandle; }

    void wren_log_with_prefix(StringView message);

    void wren_cxx_before_enter();

    void wren_cxx_before_exit();

private: //===================================================//

    WrenHandle* mScriptHandle = nullptr;

    void set_error_message(StringView method, StringView error);

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::FighterState)
