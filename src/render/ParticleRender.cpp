#include "render/ParticleRender.hpp"

#include "render/Renderer.hpp"

#include <sqee/gl/Context.hpp>

#include <sqee/redist/gl_loader.hpp>

using sq::Context;
using namespace sts;

//============================================================================//

ParticleRenderer::ParticleRenderer(Renderer& renderer) : renderer(renderer)
{
    mVertexBuffer.allocate_dynamic(sizeof(ParticleVertex) * 8192u, nullptr);
    mVertexArray.set_vertex_buffer(mVertexBuffer, sizeof(ParticleVertex));

    mVertexArray.add_float_attribute(0u, 3u, gl::FLOAT, false, 0u);
    mVertexArray.add_float_attribute(1u, 1u, gl::FLOAT, false, 12u);
    mVertexArray.add_float_attribute(2u, 3u, gl::UNSIGNED_SHORT, true, 16u);
    mVertexArray.add_float_attribute(3u, 1u, gl::UNSIGNED_SHORT, true, 22u);
    mVertexArray.add_float_attribute(4u, 1u, gl::FLOAT, false, 24u);
    mVertexArray.add_float_attribute(5u, 1u, gl::FLOAT, false, 28u);

    mTexture.load_automatic("assets/particles/Basic128");
}

//============================================================================//

void ParticleRenderer::refresh_options()
{

}

//============================================================================//

void ParticleRenderer::swap_sets()
{
    mParticleSetInfoKeep.clear();
    std::swap(mParticleSetInfo, mParticleSetInfoKeep);

    mVertices.clear();
}

void ParticleRenderer::integrate_set(float blend, const ParticleSystem& system)
{
    auto& info = mParticleSetInfo.emplace_back();

    //info.texture = renderer.resources.texarrays.acquire(system.texturePath);
    info.startIndex = uint16_t(mVertices.size());
    info.vertexCount = uint16_t(system.get_vertex_count());

    mVertices.reserve(mVertices.size() + system.get_vertex_count());
    system.compute_vertices(blend, mVertices);
}

//============================================================================//

void ParticleRenderer::render_particles()
{
    mVertexBuffer.update(0u, sizeof(ParticleVertex) * uint(mVertices.size()), mVertices.data());

    const auto compare = [](ParticleSetInfo& a, ParticleSetInfo& b) { return a.averageDepth > b.averageDepth; };
    std::sort(mParticleSetInfo.begin(), mParticleSetInfo.end(), compare);

    //--------------------------------------------------------//

    auto& context = renderer.context;

    context.bind_FrameBuffer(renderer.FB_Resolve);

    context.bind_VertexArray(mVertexArray);
    context.bind_Program(renderer.PROG_Particles);

    //gl::Enable(gl::PROGRAM_POINT_SIZE);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);
    //context.set_state(Context::Depth_Compare::LessEqual);

    context.bind_Texture(mTexture, 0u);

    context.bind_Texture(renderer.TEX_Depth, 1u);

    for (const ParticleSetInfo& info : mParticleSetInfo)
    {
        if (info.vertexCount == 0u) continue;

        //context.bind_Texture(info.texture.get(), 0u);
        gl::DrawArrays(gl::POINTS, info.startIndex, info.vertexCount);
    }
}
