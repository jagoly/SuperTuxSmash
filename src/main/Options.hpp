#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct Options final : sq::NonCopyable
{
    Options() = default;

    //--------------------------------------------------------//

    Vec2U window_size = {0u, 0u};   ///< The size of the viewport

    bool bloom_enable = true;       ///< Enable HDR Bloom effect

    uint8_t ssao_quality = 2u;      ///< Screen Space Ambient Occlusion quality

    uint8_t msaa_quality = 1u;      ///< Multi Sample Anti-Aliasing quality

    bool imgui_demo = false;        ///< Show the ImGui demo window

    //--------------------------------------------------------//

    String debug_texture = "";      ///< Debug texture to draw to screen

    bool input_disable = false;     ///< Disable all non-imgui input

    bool render_hit_blobs = false;  ///< Enable rendering of hit blobs
    bool render_hurt_blobs = false; ///< Enable rendering of hurt blobs
    bool render_diamonds = false;   ///< Enable rendering of diamonds
    bool render_skeletons = false;  ///< Enable rendering of skeletons

    bool camera_smooth = true;      ///< Smoothly move the camera over time
    float camera_zoom_out = 1.f;    ///< Minimum distance between camera and scene

    bool log_animation = false;     ///< Enable logging of animation stuff
    bool log_input = false;         ///< Enable logging of input stuff
    bool log_script = false;        ///< Enable logging of script stuff

    bool editor_mode = false;       ///< CANNOT BE TOGGLED, SHOULD NOT BE HERE

    bool debug_toggle_1 = false;    ///< Used for whatever, press 1
    bool debug_toggle_2 = false;    ///< Used for whatever, press 2

    //--------------------------------------------------------//

    void validate() const;  ///< Assert that options are valid.
};

//============================================================================//

} // namespace sts
