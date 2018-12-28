#include "main/Options.hpp"

#include <sqee/debug/Logging.hpp>

#include <unordered_set>

//============================================================================//

void sts::Options::validate() const
{
    sq::log_assert(Window_Size.x >= 640u,  "horizontal resolution too low");
    sq::log_assert(Window_Size.y >= 360u,  "vertical resolution too low");
    sq::log_assert(Window_Size.x <= 2560u, "horizontal resolution too high");
    sq::log_assert(Window_Size.y <= 1600u, "vertical resolution too high");
    sq::log_assert(SSAO_Quality  <= 2u,    "SSAO_Quality too high");
    sq::log_assert(MSAA_Quality  <= 2u,    "MSAA_Quality too high");

    static const std::unordered_set<String> validDebugTextures
    {
        "", "depth", "ssao", "bloom"
    };

    sq::log_assert(validDebugTextures.count(Debug_Texture), "Debug_Texture not recognised");
}
