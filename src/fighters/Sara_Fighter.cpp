#include "fighters/Sara_Fighter.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Sara)
{
    diamond = LocalDiamond(0.3f, 1.4f, 0.8f);
}

//============================================================================//

void Sara_Fighter::tick()
{
    base_tick_fighter();

    base_tick_animation();

    //--------------------------------------------------------//

    ParticleEmitter emitter;

    emitter.shape = ParticleEmitter::DiscShape();

    emitter.emitPosition = Vec3F(get_position(), 0.f);
    emitter.emitVelocity = Vec3F(get_velocity().x * 0.2f, 0.f, 0.f);
    emitter.direction = Vec3F(0.f, 1.f, 0.f);

    emitter.endScale = 2.f;
    emitter.endOpacity = 0.f;

    emitter.lifetime = { 16u, 24u };
    emitter.radius = { 0.4f, 0.6f };
    emitter.opacity = { 0.4f, 0.5f };

    emitter.colour_fixed() = { 1.f, 1.f, 1.f };

    emitter.shape_disc().incline = { -0.01f, 0.04f };
    emitter.shape_disc().speed = { 1.2f, 2.2f };

    emitter.sprite = "Smoke";

    auto& partilcles = mFightWorld.get_particle_system();

    if (current.state == State::Landing && previous.state != State::Landing)
    {
        emitter.emitPosition.z = -0.2f;
        emitter.generate(partilcles, 20u);
        emitter.emitPosition.z = +0.2f;
        emitter.generate(partilcles, 20u);
    }
}
