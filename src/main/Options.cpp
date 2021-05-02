#include "main/Options.hpp"

#include <sqee/debug/Logging.hpp>

//============================================================================//

void sts::Options::validate() const
{
    //if (window_size.x >= 640u)  sq::log_error("window_size.x too low");
    //if (window_size.x <= 2560u) sq::log_error("window_size.x too high");
    //if (window_size.y >= 360u)  sq::log_error("window_size.y too low");
    //if (window_size.y <= 1600u) sq::log_error("window_size.y too high");
    if (ssao_quality  <= 2u)    sq::log_error("ssao_quality too high");
    if (msaa_quality  <= 2u)    sq::log_error("msaa_quality too high");

    const auto validDebugTextures = { "", "depth", "ssao", "bloom" };
    if (algo::find(validDebugTextures, debug_texture) == validDebugTextures.end())
        sq::log_error("debug_texture not recognised");
}
