#pragma once

#include <sqee/misc/Builtins.hpp>

// todo: there's no reason for the action editor to not be a part of the release build

namespace sts {

struct DebugGlobals
{
    bool disableInput = false;
    bool renderBlobs = false;
    bool actionEditor = false;
};

extern DebugGlobals dbg;

} // namespace sts
