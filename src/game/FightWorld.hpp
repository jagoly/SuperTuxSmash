#pragma once

#include <sqee/redist/sol.hpp>

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Builtins.hpp>
#include <sqee/misc/PoolTools.hpp>
#include <sqee/app/MessageBus.hpp>

#include "main/Enumerations.hpp"
#include "main/Globals.hpp"

#include "render/SceneData.hpp"

#include "game/Blobs.hpp"
#include "game/ParticleSystem.hpp"
#include "game/ParticleEmitter.hpp"

struct WrenVM;

namespace sts {

//============================================================================//

struct LocalDiamond
{
    LocalDiamond() = default;

    LocalDiamond(float _halfWidth, float _offsetTop, float _offsetCross)
    {
        halfWidth = _halfWidth;
        offsetTop = _offsetTop;
        offsetCross = _offsetCross;

        normLeftDown = sq::maths::normalize(Vec2F(-_offsetCross, -_halfWidth));
        normLeftUp = sq::maths::normalize(Vec2F(-_offsetCross, +_halfWidth));
        normRightDown = sq::maths::normalize(Vec2F(+_offsetCross, -_halfWidth));
        normRightUp = sq::maths::normalize(Vec2F(+_offsetCross, +_halfWidth));
    }

    float halfWidth, offsetTop, offsetCross;
    Vec2F normLeftDown, normLeftUp, normRightDown, normRightUp;

    Vec2F min() const { return { -halfWidth, 0.f }; }
    Vec2F max() const { return { +halfWidth, offsetTop }; }
    Vec2F cross() const { return { 0.f, offsetCross }; }
};

//============================================================================//

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld(const Globals& globals);

    ~FightWorld();

    //--------------------------------------------------------//

    const Globals& globals;

    sol::state lua;

    //--------------------------------------------------------//

    void tick();

    //--------------------------------------------------------//

    /// Set the stage for the game.
    void set_stage(UniquePtr<Stage> stage);

    /// Add a fighter to the game.
    void add_fighter(UniquePtr<Fighter> fighter);

    //--------------------------------------------------------//

    /// Enable a hit blob.
    void enable_hit_blob(HitBlob* blob);

    /// Disable a hit blob.
    void disable_hit_blob(HitBlob* blob);

    /// Enable a hurt blob.
    void enable_hurt_blob(HurtBlob* blob);

    /// Disable a hurt blob.
    void disable_hurt_blob(HurtBlob* blob);

    //--------------------------------------------------------//

    /// Reset all of a fighter's hit blob groups.
    void reset_all_hit_blob_groups(Fighter& fighter);

    /// Disable all of a fighter's hit blobs.
    void disable_all_hit_blobs(Fighter& fighter);

    /// Disable all of a fighter's hurt blobs.
    void disable_all_hurt_blobs(Fighter& fighter);

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

    /// Access the HurtBlob Allocator.
    auto get_hurt_blob_allocator() { return mHurtBlobAlloc.get(); }

    /// Access the HitBlob Allocator.
    auto get_hit_blob_allocator() { return mHitBlobAlloc.get(); }

    /// Access the Emitter Allocator.
    auto get_emitter_allocator() { return mEmitterAlloc.get(); }

    /// Access the enabled HurtBlobs.
    const Vector<HurtBlob*>& get_hurt_blobs() const;

    /// Access the enabled HitBlobs.
    const Vector<HitBlob*>& get_hit_blobs() const;

    //--------------------------------------------------------//

    /// Access the ParticleSystem.
    ParticleSystem& get_particle_system() { return mParticleSystem; }

    /// Access the MessageBus.
    sq::MessageBus& get_message_bus() { return mMessageBus; }

    //--------------------------------------------------------//

    SceneData compute_scene_data() const;

private: //===================================================//

    // undo stack in the editor can cause us to run out of slots, so we reserve more space
    // todo: for the editor, use big pools shared between all contexts

    sq::PoolAllocatorStore<Pair<const TinyString, HurtBlob>> mHurtBlobAlloc;
    sq::PoolAllocatorStore<Pair<const TinyString, HitBlob>> mHitBlobAlloc;
    sq::PoolAllocatorStore<Pair<const TinyString, ParticleEmitter>> mEmitterAlloc;

    //--------------------------------------------------------//

    ParticleSystem mParticleSystem;

    sq::MessageBus mMessageBus;

    //--------------------------------------------------------//

    UniquePtr<Stage> mStage;

    Array<UniquePtr<Fighter>, 4> mFighters;

    //--------------------------------------------------------//

    friend class PrivateWorld;
    UniquePtr<PrivateWorld> impl;

public: //== debug and editor interfaces =====================//

    void debug_show_fighter_widget();

    void debug_reload_actions();

    void editor_disable_all_hurtblobs();
};

//============================================================================//

} // namespace sts
