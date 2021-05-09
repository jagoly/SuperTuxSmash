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
        // todo: we want this to be evenly distributed, not random
        const Vec3F ballDirection = maths::normalize(randNormal(mGenerator));
        p.currentPos += ballDirection * emitter.ballOffset(mGenerator);
        p.velocity += ballDirection * emitter.ballSpeed(mGenerator);

        // apply disc shape modifiers
        const float discIncline = emitter.discIncline(mGenerator);
        const float discAngle = float(i) / float(emitter.count);
        const Vec3F discDirection = maths::rotate_y(maths::rotate_x(Vec3F(0, 0, -1), discIncline), discAngle);
        p.currentPos += Mat3F(boneMatrix) * discDirection * emitter.discOffset(mGenerator);
        p.velocity += Mat3F(boneMatrix) * discDirection * emitter.discSpeed(mGenerator);
    }
}

//============================================================================//

void ParticleSystem::update_and_clean()
{
    const auto predicate = [](const ParticleData& p) { return p.progress == p.lifetime; };
    algo::erase_if(mParticles, predicate);

    for (ParticleData& p : mParticles)
    {
        p.progress += 1u;
        p.previousPos = p.currentPos;
        p.currentPos += p.velocity;
        p.velocity -= p.velocity * p.friction;
    }

    const auto compare = [](const ParticleData& a, const ParticleData& b) { return a.currentPos.z > b.currentPos.z; };
    std::sort(mParticles.begin(), mParticles.end(), compare);
}
