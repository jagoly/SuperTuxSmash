#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

namespace sts {

//============================================================================//

struct Environment
{
    struct {
        float exposure;
        float contrast;
        float black;
    } tonemap;

    struct {
        TextureHandle skybox;
        TextureHandle irradiance;
        TextureHandle radiance;
    } cubemaps;
};

//============================================================================//

} // namespace sts
