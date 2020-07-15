#pragma once

#include <sqee/setup.hpp>

namespace sts {

/// SuperTuxSmash Globals.
class Globals final : sq::NonCopyable
{
public: //====================================================//

    Globals() = default;

    //--------------------------------------------------------//

    bool disableInput = false;

    bool renderHitBlobs = false;
    bool renderHurtBlobs = false;
    bool renderDiamonds = false;
    bool renderSkeletons = false;

    bool editorMode = false;
};

} // namespace sts
