#pragma once

#include "game/Actions.hpp"

namespace sts {

//============================================================================//

struct ActionBuilder final
{
    static optional<Action::Command> build_command(Action& action, string_view source);

    static void load_from_json(Action& action);
};

//============================================================================//

} // namespace sts
