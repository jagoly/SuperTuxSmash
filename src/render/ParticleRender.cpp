#include <algorithm>

#include <sqee/gl/Context.hpp>

#include "render/ParticleRender.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

ParticleRender::ParticleRender(Renderer& renderer) : renderer(renderer)
{
    mVertexBuffer.allocate_dynamic(24u * 8192u, nullptr);
    mVertexArray.set_vertex_buffer(mVertexBuffer, 24u);

    mVertexArray.add_float_attribute(0u, 3u, gl::FLOAT, false, 0u);
    mVertexArray.add_float_attribute(1u, 1u, gl::FLOAT, false, 12u);
    mVertexArray.add_float_attribute(2u, 1u, gl::FLOAT, false, 16u);
    mVertexArray.add_float_attribute(3u, 1u, gl::FLOAT, false, 20u);

    mTexture.load_automatic("particles/Basic128");
}

//============================================================================//

void ParticleRender::refresh_options()
{

}

//============================================================================//

void ParticleRender::swap_sets()
{
    mParticleSetInfoKeep.clear();
    std::swap(mParticleSetInfo, mParticleSetInfoKeep);

    mVertices.clear();
}

void ParticleRender::integrate_set(float blend, const ParticleSystem& system)
{
    auto& info = mParticleSetInfo.emplace_back();

    //info.texture = renderer.resources.texarrays.acquire(system.texturePath);
    info.startIndex = uint16_t(mVertices.size());
    info.vertexCount = uint16_t(system.get_particles().size());
    system.compute_vertices(blend, mVertices);
}

//============================================================================//

void ParticleRender::render_particles()
{
    mVertexBuffer.update(0u, 24u * uint(mVertices.size()), mVertices.data());

    const auto compare = [&](ParticleSetInfo& a, ParticleSetInfo& b) { return a.averageDepth > b.averageDepth; };
    std::sort(mParticleSetInfo.begin(), mParticleSetInfo.end(), compare);

    //--------------------------------------------------------//

    auto& context = renderer.context;

    context.bind_VertexArray(mVertexArray);
    context.bind_Program(renderer.shaders.Particles);

    gl::Enable(gl::PROGRAM_POINT_SIZE);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Keep);
    context.set_state(Context::Depth_Compare::LessEqual);

    context.bind_Texture(mTexture, 0u);

    for (const ParticleSetInfo& info : mParticleSetInfo)
    {
        if (info.vertexCount == 0u) continue;

        //context.bind_Texture(info.texture.get(), 0u);
        gl::DrawArrays(gl::POINTS, info.startIndex, info.vertexCount);
    }
}
