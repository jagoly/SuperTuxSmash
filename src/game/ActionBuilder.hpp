#pragma once

#include <sqee/misc/Json.hpp>

#include "game/Actions.hpp"

namespace sts {

//============================================================================//

class ActionBuilder final : sq::NonCopyable
{
public:

    Action::Command build_command(Action& action, StringView source);

    Vector<Action::Command> build_procedure(Action& action, StringView source);

    void load_from_json(Action& action);

    JsonValue serialise_as_json(const Action& action);

    void flush_logged_errors(String heading);

private:

    Vector<String> mErrorLog;

    template <class... Args>
    void impl_log_error(const char* fmt, const Args&... args);
};

//============================================================================//

} // namespace sts
