#include <unordered_set>

#include <sqee/debug/Logging.hpp>

#include "Options.hpp"

using namespace sts;

//============================================================================//

void Options::validate() const
{
    sq::log_assert(Window_Size.x >= 640u,  "horizontal resolution too low");
    sq::log_assert(Window_Size.y >= 360u,  "vertical resolution too low");
    sq::log_assert(Window_Size.x <= 2560u, "horizontal resolution too high");
    sq::log_assert(Window_Size.y <= 1600u, "vertical resolution too high");
    sq::log_assert(SSAO_Quality  <= 2u,    "SSAO_Quality too high");
    sq::log_assert(FSAA_Quality  <= 2u,    "FSAA_Quality too high");

    static const std::unordered_set<string> validDebugTextures
    {
        "", "diffuse", "surface", "normals", "specular", "lighting", "ssao", "bloom"
    };

    sq::log_assert(validDebugTextures.count(Debug_Texture), "Debug_Texture not recognised");
}

//============================================================================//

Options& Options::get()
{
    static Options instance;
    return instance;
}
