#pragma once

#include "game/Actions.hpp"

namespace sts {

//============================================================================//

struct ActionBuilder final
{
    static std::function<void(Action& action)> build_command(Action& action, StringView source);

    static void load_from_json(Action& action);

    static JsonValue serialise_as_json(const Action& action);
};

//============================================================================//

} // namespace sts
