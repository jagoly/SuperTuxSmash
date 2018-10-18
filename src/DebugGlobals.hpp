#pragma once

#include <sqee/misc/Builtins.hpp>

#ifdef SQEE_DEBUG

namespace sts {

struct DebugGlobals
{
    bool disableInput = false;
    bool renderBlobs = false;
};

extern DebugGlobals dbg;

} // namespace sts

#endif
