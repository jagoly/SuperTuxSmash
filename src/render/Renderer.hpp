#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include "render/DrawItem.hpp"

#include <sqee/gl/FixedBuffer.hpp>
#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>

//============================================================================//

namespace sq { class Context; }

namespace sts {

class DebugRenderer;
class ParticleRenderer;

//============================================================================//

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const Options& options, sq::PreProcessor& processor, ResourceCaches& caches);

    ~Renderer();

    //--------------------------------------------------------//

    sq::Context& context;

    const Options& options;

    sq::PreProcessor& processor;

    ResourceCaches& caches;

    //--------------------------------------------------------//

    void refresh_options();

    //--------------------------------------------------------//

    void set_camera(std::unique_ptr<Camera> camera);

    Camera& get_camera() { return *mCamera; }

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    /// Create some DrawItems from a vector of definitions.
    int64_t create_draw_items(const std::vector<DrawItemDef>& defs, const sq::FixedBuffer* ubo,
                              std::map<TinyString, const bool*> conditions);

    /// Delete all DrawItems with the given group id.
    void delete_draw_items(int64_t groupId);

    //--------------------------------------------------------//

    void render_objects(float blend);

    void render_particles(const ParticleSystem& system, float blend);

    void resolve_multisample();

    void finish_rendering();

    //--------------------------------------------------------//

    DebugRenderer& get_debug_renderer() { return *mDebugRenderer; }

    //--------------------------------------------------------//

    sq::FrameBuffer FB_MsMain;
    sq::FrameBuffer FB_Resolve;

    sq::TextureMulti TEX_MsDepth;
    sq::TextureMulti TEX_MsColour;
    sq::Texture2D TEX_Depth;
    sq::Texture2D TEX_Colour;

    sq::Program PROG_Particles;
    sq::Program PROG_Lighting_Skybox;
    sq::Program PROG_Composite;

private: //===================================================//

    std::unique_ptr<Camera> mCamera;

    std::unique_ptr<DebugRenderer> mDebugRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;

    sq::FixedBuffer mLightUbo;

    std::vector<DrawItem> mDrawItems;

    int64_t mCurrentGroupId = -1;
};

//============================================================================//

} // namespace sts
