#include "fighters/Sara_Fighter.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, "Sara")
{
    mLocalDiamond.offsetTop = 1.4f;
    mLocalDiamond.offsetMiddle = 0.8f;
    mLocalDiamond.halfWidth = 0.3f;
}

//============================================================================//

void Sara_Fighter::tick()
{
    base_tick_fighter();

    base_tick_animation();

    //--------------------------------------------------------//

    ParticleEmitter emitter;

    emitter.shape = ParticleEmitter::DiscShape();

    emitter.emitPosition = Vec3F(get_diamond().origin(), 0.f);
    emitter.emitVelocity = Vec3F(get_velocity().x * 0.2f, 0.f, 0.f);
    emitter.direction = Vec3F(0.f, 1.f, 0.f);

    emitter.lifetime = { 16u, 24u };
    emitter.scale = { 0.3f, 0.6f };

    emitter.radius = { 0.5f, 1.0f };
    emitter.opacity = { 0.4f, 0.0f };

    emitter.disc().incline = { -0.01f, 0.04f };
    emitter.disc().speed = { 1.1f, 1.8f };

//    ParticleGeneratorColumn generator;

//    generator.emitPosition = Vec3F(get_diamond().yNeg, 0.f);
//    generator.emitVelocity = Vec3F(get_velocity() * 0.1f, 0.5f);
//    generator.direction = Vec3F(0.f, 1.f, 0.f);
//    generator.scale = { 0.8f, 1.2f };
//    generator.lifetime = { 48u, 80u };
//    generator.deviation = { 1.3f, 1.5f };
//    generator.speed = { 1.5f, 3.5f };

    auto& partilcles = mFightWorld.get_particle_set();

    partilcles.texturePath = "particles/WhitePuff";

    //mParticleSet.texturePath = "particles/WhitePuff";
    //mParticleSet.friction = 0.02f;
    //mParticleSet.radius = { 0.5f, 1.f };
    //mParticleSet.opacity = { 0.4f, 0.f };

    if (current.state == State::Landing && previous.state != State::Landing)
        emitter.generate(partilcles, 40u);

    //partilcles.update_and_clean();
}
