#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

class FighterState final : sq::NonCopyable
{
public: //====================================================//

    FighterState(Fighter& fighter, SmallString name);

    ~FighterState();

    //--------------------------------------------------------//

    Fighter& fighter;
    FightWorld& world;

    const TinyString name;

    //--------------------------------------------------------//

    void call_do_enter();

    void call_do_updates();

    void call_do_exit();

    //--------------------------------------------------------//

    void load_wren_from_file();

    //-- wren methods ----------------------------------------//

    Fighter* wren_get_fighter() { return &fighter; }

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
