#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/maths/Builtins.hpp>

//============================================================================//

namespace sts {

/// SuperTuxSmash Options.
class Options final : sq::NonCopyable
{
public: //====================================================//

    Options() = default;

    //--------------------------------------------------------//

    /// The Size of the Viewport.
    Vec2U Window_Size = {800u, 600u};

    /// Enable HDR Bloom Effect.
    bool Bloom_Enable = true;

    /// Screen Space Ambient Occlusion Quality.
    uchar SSAO_Quality = 2u;

    /// Multi Sample Anti-Aliasing Quality.
    uchar MSAA_Quality = 1u;

    /// Debug Texture to Draw to Screen.
    String Debug_Texture = "";

    //--------------------------------------------------------//

    /// Assert that options are valid.
    void validate() const;
};

} // namespace sts
