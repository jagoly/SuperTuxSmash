#pragma once

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Builtins.hpp>
#include <sqee/app/MessageBus.hpp>

#include "render/SceneData.hpp"

#include "game/Blobs.hpp"
#include "game/ParticleSystem.hpp"
#include "game/ParticleEmitter.hpp"
#include "game/ActionBuilder.hpp"

#include "enumerations.hpp"

namespace sts {

//============================================================================//

struct LocalDiamond { float offsetTop, offsetMiddle, halfWidth; };

struct WorldDiamond
{
    float negX, posX, negY, posY, crossX, crossY;

    Vec2F neg_x() const { return { negX, crossY }; }
    Vec2F pos_x() const { return { posX, crossY }; }
    Vec2F neg_y() const { return { crossX, negY }; }
    Vec2F pos_y() const { return { crossX, posY }; }

    Vec2F origin() const { return { crossX, negY }; }
    Vec2F centre() const { return { crossX, crossY }; }

    WorldDiamond translated(Vec2F vec) const
    {
        return { negX + vec.x, posX + vec.x, negY + vec.y, posY + vec.y,
                 crossX + vec.x, crossY + vec.y };
    }
};

//============================================================================//

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld(GameMode gameMode);

    ~FightWorld();

    //--------------------------------------------------------//

    void tick();

    //--------------------------------------------------------//

    /// Get the mode of the active game.
    GameMode get_game_mode() const { return mGameMode; }

    //--------------------------------------------------------//

    /// Set the stage for the game.
    void set_stage(UniquePtr<Stage> stage);

    /// Add a fighter to the game.
    void add_fighter(UniquePtr<Fighter> fighter);

    //--------------------------------------------------------//

    /// Create a new hurt blob.
    HurtBlob* create_hurt_blob(Fighter& fighter);

    /// Delete an existing hurt blob.
    void delete_hurt_blob(HurtBlob* blob);

    //--------------------------------------------------------//

    /// Enable a hit blob.
    void enable_hit_blob(HitBlob* blob);

    /// Disable a hit blob.
    void disable_hit_blob(HitBlob* blob);

    //--------------------------------------------------------//

    /// Reset all of a fighter's hit blob groups.
    void reset_all_hit_blob_groups(Fighter& fighter);

    /// Disable all of a fighter's hit blobs.
    void disable_all_hit_blobs(Fighter& fighter);

    //--------------------------------------------------------//

    /// Access the stage.
    Stage& get_stage() { SQASSERT(mStage, ""); return *mStage; }

    /// Access the stage (const).
    const Stage& get_stage() const { SQASSERT(mStage, ""); return *mStage; }

    /// Access a fighter by index, might be null.
    Fighter* get_fighter(uint8_t index) { return mFighters[index].get(); }

    /// Access a fighter by index, might be null (const).
    const Fighter* get_fighter(uint8_t index) const { return mFighters[index].get(); }

    //--------------------------------------------------------//

    /// Acquire a vector of all added fighters.
    Vector<Fighter*> get_fighters()
    {
        Vector<Fighter*> result;
        result.reserve(4u);

        for (auto& uptr : mFighters)
            if (uptr != nullptr)
                result.push_back(uptr.get());

        return result;
    }

    //--------------------------------------------------------//

    /// Access the HitBlob Allocator.
    sq::PoolAllocator<HitBlob>& get_hit_blob_allocator() { return mHitBlobAlloc; }

    /// Access the HurtBlob Allocator.
    sq::PoolAllocator<HurtBlob>& get_hurt_blob_allocator() { return mHurtBlobAlloc; }

    /// Access the enabled HitBlobs.
    const Vector<HitBlob*>& get_hit_blobs() const;

    /// Access the enabled HurtBlobs.
    const Vector<HurtBlob*>& get_hurt_blobs() const;

    /// Access the Emitter Allocator.
    sq::PoolAllocator<ParticleEmitter>& get_emitter_allocator() { return mEmitterAlloc; }

    //--------------------------------------------------------//

    /// Access the ActionBuilder.
    ActionBuilder& get_action_builder() { return mActionBuilder; }

    /// Access the ParticleSystem.
    ParticleSystem& get_particle_system() { return mParticleSystem; }

    /// Access the MessageBus.
    sq::MessageBus& get_message_bus() { return mMessageBus; }

    //--------------------------------------------------------//

    SceneData compute_scene_data() const;

private: //===================================================//

    sq::PoolAllocator<HitBlob> mHitBlobAlloc { 1024u };
    sq::PoolAllocator<HurtBlob> mHurtBlobAlloc { 128u };

    sq::PoolAllocator<ParticleEmitter> mEmitterAlloc { 1024u };

    //--------------------------------------------------------//

    const GameMode mGameMode;

    ActionBuilder mActionBuilder;

    ParticleSystem mParticleSystem;

    sq::MessageBus mMessageBus;

    //--------------------------------------------------------//

    UniquePtr<Stage> mStage;

    Array<UniquePtr<Fighter>, 4> mFighters;

    //--------------------------------------------------------//

    friend class PrivateWorld;
    UniquePtr<PrivateWorld> impl;
};

//============================================================================//

} // namespace sts
