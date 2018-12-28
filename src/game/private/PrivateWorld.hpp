#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/misc/PoolTools.hpp>

#include "render/SceneData.hpp"

#include "game/Blobs.hpp"
#include "game/ParticleSystem.hpp"
#include "game/ParticleEmitter.hpp"

//============================================================================//

namespace sts {

class PrivateWorld final : sq::NonCopyable
{
public: //====================================================//

    PrivateWorld(FightWorld& world) : world(world) {}

    ~PrivateWorld() = default;

    //--------------------------------------------------------//

    void tick();

    //--------------------------------------------------------//

    Vector<HitBlob*> enabledHitBlobs;
    Vector<HurtBlob*> enabledHurtBlobs;

    Array<Array<uint32_t, 4>, 4> hitBitsArray;

private: //===================================================//

    struct Collision { HitBlob* hit; HurtBlob* hurt; };

    Array<Array<Vector<Collision>, 4u>, 4u> mCollisions;

    //--------------------------------------------------------//

    FightWorld& world;
};

} // namespace sts
