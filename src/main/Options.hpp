#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

//============================================================================//

namespace sts {

/// SuperTuxSmash Options
class Options final : sq::NonCopyable
{
public: //====================================================//

    Options() = default;

    //--------------------------------------------------------//

    /// The Size of the Viewport
    Vec2U Window_Size = {800u, 600u};

    /// Enable HDR Bloom Effect
    bool Bloom_Enable = true;

    /// Screen Space Ambient Occlusion Quality
    uchar SSAO_Quality = 2u;

    /// Full Screen Anti-Aliasing Quality
    uchar FSAA_Quality = 2u;

    /// Debug Texture to Draw to Screen
    string Debug_Texture = "";

    //--------------------------------------------------------//

    /// Assert that options are valid
    void validate() const;
};

} // namespace sts
