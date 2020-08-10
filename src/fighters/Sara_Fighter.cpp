#include "fighters/Sara_Fighter.hpp"

#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Sara) {}

//============================================================================//

void Sara_Fighter::tick()
{
    base_tick_fighter();

    //--------------------------------------------------------//

    Emitter emitter;

    emitter.fighter = this;

    emitter.sprite = "Smoke";
    emitter.colour = { Vec3F(1.f, 1.f, 1.f) };

    emitter.baseOpacity = 0.5f;
    emitter.endOpacity = 0.f;
    emitter.endScale = 2.f;

    emitter.lifetime = { 16u, 24u };
    emitter.baseRadius = { 0.4f, 0.6f };

    emitter.discIncline = { -0.01f, 0.04f };
    emitter.discSpeed = { 1.2f, 2.2f };

    //auto& partilcles = world.get_particle_system();

    /*if (current.state == State::Landing && previous.state != State::Landing)
    {
        emitter.emitPosition.z = -0.2f;
        emitter.generate(partilcles, 20u);
        emitter.emitPosition.z = +0.2f;
        emitter.generate(partilcles, 20u);
    }*/
}
