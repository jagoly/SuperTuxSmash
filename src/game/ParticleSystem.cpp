#include "game/ParticleSystem.hpp"

#include "game/Emitter.hpp"
#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/maths/Random.hpp>

using namespace sts;

//============================================================================//

ParticleSystem::ParticleSystem()
{
    sprites =
    {
        { "Smoke", { 0, 15 } },
        { "Sparkle", { 16, 19 } }
    };
}

//============================================================================//

void ParticleSystem::generate(const Emitter& emitter)
{
    SQASSERT(emitter.bone == -1 || emitter.fighter != nullptr, "bone set without a fighter");

    const auto& [spriteMin, spriteMax] = sprites[emitter.sprite];

    maths::RandomRange<uint16_t> randSprite { spriteMin, spriteMax };
    maths::RandomRange<uint16_t> randColour { 0u, uint16_t(emitter.colour.size() - 1u) };

    maths::RandomRange<Vec3F> randNormal { Vec3F(-1.f), Vec3F(1.f) };
    maths::RandomRange<float> randAngle { 0.f, 1.f };

    const Mat4F boneMatrix = emitter.fighter ? emitter.fighter->get_bone_matrix(emitter.bone) : Mat4F();

    //--------------------------------------------------------//

    for (uint i = 0u; i < emitter.count; ++i)
    {
        auto& p = mParticles.emplace_back();

        p.progress = 0u;

        p.currentPos = Vec3F();
        p.velocity = Vec3F();

        p.baseOpacity = emitter.baseOpacity;
        p.endOpacity = emitter.endOpacity;
        p.endScale = emitter.endScale;

        p.lifetime = emitter.lifetime(mGenerator);
        p.baseRadius = emitter.baseRadius(mGenerator);

        p.sprite = randSprite(mGenerator);
        p.colour = emitter.colour[uint8_t(randColour(mGenerator))];

        p.friction = 0.1f;

        // apply shapeless launch offset and velocity
        p.currentPos += Vec3F(boneMatrix * Vec4F(emitter.origin, 1.f));
        p.velocity += Mat3F(boneMatrix) * emitter.velocity;

        // apply ball shape modifiers
        const Vec3F ballDirection = maths::normalize(randNormal(mGenerator));
        p.currentPos += ballDirection * emitter.ballOffset(mGenerator);
        p.velocity += ballDirection * emitter.ballSpeed(mGenerator);

        // apply disc shape modifiers
        const Vec3F discDirection = maths::rotate_y(maths::rotate_x(Vec3F(0, 0, -1), emitter.discIncline(mGenerator)), randAngle(mGenerator));
        p.currentPos += Mat3F(boneMatrix) * discDirection * emitter.discOffset(mGenerator);
        p.velocity += Mat3F(boneMatrix) * discDirection * emitter.discSpeed(mGenerator);

        // do this once currentPos is ready
        p.previousPos = p.currentPos;
    }
}

//============================================================================//

void ParticleSystem::update_and_clean()
{
    for (ParticleData& p : mParticles)
    {
        p.previousPos = p.currentPos;
        p.currentPos += p.velocity;
        p.velocity -= p.velocity * p.friction;
    }

    const auto predicate = [](ParticleData& p) { return ++p.progress == p.lifetime; };
    algo::erase_if(mParticles, predicate);

    const auto compare = [](const ParticleData& a, const ParticleData& b) { return a.currentPos.z > b.currentPos.z; };
    std::sort(mParticles.begin(), mParticles.end(), compare);
}

//============================================================================//

void ParticleSystem::compute_vertices(float blend, VertexVec& vertices) const
{
    const auto UNorm16 = [](float value) { return uint16_t(value * 65535.0f); };

    for (const ParticleData& p : mParticles)
    {
        ParticleVertex& vertex = vertices.emplace_back();

        const float factor = (float(p.progress) + blend) / float(p.lifetime);

        vertex.position = maths::mix(p.previousPos, p.currentPos, blend);
        vertex.radius = p.baseRadius * maths::mix(1.f, p.endScale, factor);
        vertex.colour[0] = UNorm16(p.colour.r);
        vertex.colour[1] = UNorm16(p.colour.g);
        vertex.colour[2] = UNorm16(p.colour.b);
        vertex.opacity = UNorm16(std::pow(p.baseOpacity * maths::mix(1.f, p.endOpacity, factor), 0.5f));
        vertex.misc = 0.f;
        vertex.index = float(p.sprite);
    }
}
