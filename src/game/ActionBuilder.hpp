#pragma once

#include <sqee/misc/Json.hpp>

#include "game/Actions.hpp"

namespace sts {

//============================================================================//

class ActionBuilder final : sq::NonCopyable
{
public:

    Action::Command build_command(Action& action, StringView source, Vector<String>& errors, uint line);

    Vector<Action::Command> build_procedure(Action& action, StringView source, Vector<String>& errors);

    void load_from_json(Action& action);

    JsonValue serialise_as_json(const Action& action);
};

//============================================================================//

} // namespace sts
