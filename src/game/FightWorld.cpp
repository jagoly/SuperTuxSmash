#include "game/FightWorld.hpp"

#include "main/Options.hpp"

#include "game/Action.hpp"
#include "game/Controller.hpp"
#include "game/Fighter.hpp"
#include "game/ParticleSystem.hpp"
#include "game/Stage.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Culling.hpp>

using namespace sts;

//============================================================================//

FightWorld::FightWorld(const Options& options, sq::AudioContext& audio)
    : options(options)
    , audio(audio)
    , sounds(audio)
    , mHurtBlobAlloc    (options.editor_mode ? 1280 : 128)
    , mHitBlobAlloc     (options.editor_mode ? 10240 : 1024)
    , mEmitterAlloc     (options.editor_mode ? 5120 : 512)
    , mSoundEffectAlloc (options.editor_mode ? 5120 : 512)
{
    vm.add_foreign_method<&Action::wren_set_wait_until>("set_wait_until(_)");
    vm.add_foreign_method<&Action::wren_allow_interrupt>("allow_interrupt()");
    vm.add_foreign_method<&Action::wren_enable_hitblob_group>("enable_hitblob_group(_)");
    vm.add_foreign_method<&Action::wren_disable_hitblob_group>("disable_hitblob_group(_)");
    vm.add_foreign_method<&Action::wren_disable_hitblobs>("disable_hitblobs()");
    vm.add_foreign_method<&Action::wren_emit_particles>("emit_particles(_)");
    vm.add_foreign_method<&Action::wren_play_sound>("play_sound(_)");

    vm.add_foreign_method<&Fighter::wren_set_intangible>("set_intangible(_)");
    vm.add_foreign_method<&Fighter::wren_enable_hurtblob>("enable_hurtblob(_)");
    vm.add_foreign_method<&Fighter::wren_disable_hurtblob>("disable_hurtblob(_)");
    vm.add_foreign_method<&Fighter::wren_set_velocity_x>("set_velocity_x(_)");
    vm.add_foreign_method<&Fighter::wren_set_autocancel>("set_autocancel(_)");

    handles.script_new = wrenMakeCallHandle(vm, "new(_,_)");
    handles.script_reset = wrenMakeCallHandle(vm, "reset()");
    handles.script_cancel = wrenMakeCallHandle(vm, "cancel()");
    handles.fiber_call = wrenMakeCallHandle(vm, "call()");
    handles.fiber_isDone = wrenMakeCallHandle(vm, "isDone");

    mParticleSystem = std::make_unique<ParticleSystem>();
}

FightWorld::~FightWorld()
{
    wrenReleaseHandle(vm, handles.script_new);
    wrenReleaseHandle(vm, handles.script_reset);
    wrenReleaseHandle(vm, handles.script_cancel);
    wrenReleaseHandle(vm, handles.fiber_call);
    wrenReleaseHandle(vm, handles.fiber_isDone);
}

//============================================================================//

void FightWorld::finish_setup()
{
    if (mFighterRefs.size() == 1u)
    {
        // leave the single fighter where they are
    }
    else if (mFighterRefs.size() == 2u)
    {
        mFighterRefs[0]->status.position.x = -2.f;
        mFighterRefs[1]->status.position.x = +2.f;
        mFighterRefs[1]->status.facing = -1;
    }
    else if (mFighterRefs.size() == 3u)
    {
        mFighterRefs[0]->status.position.x = -4.f;
        mFighterRefs[2]->status.position.x = +4.f;
        mFighterRefs[2]->status.facing = -1;
    }
    else if (mFighterRefs.size() == 4u)
    {
        mFighterRefs[0]->status.position.x = -6.f;
        mFighterRefs[1]->status.position.x = -2.f;
        mFighterRefs[2]->status.position.x = +2.f;
        mFighterRefs[2]->status.facing = -1;
        mFighterRefs[3]->status.position.x = +6.f;
        mFighterRefs[3]->status.facing = -1;
    }
}

//============================================================================//

void FightWorld::tick()
{
    mStage->tick();

    for (auto& fighter : mFighters)
    {
        if (fighter != nullptr)
            fighter->tick();
    }

    impl_update_collisions();

    for (auto& fighter : mFighters)
    {
        if (fighter != nullptr)
            mStage->check_boundary(*fighter);
    }

    mParticleSystem->update_and_clean();
}

//============================================================================//

void FightWorld::impl_update_collisions()
{
    //-- update world space shapes of hit and hurt blobs -----//

    for (HitBlob* blob : mEnabledHitBlobs)
    {
        const Mat4F matrix = blob->action->fighter.get_bone_matrix(blob->bone);

        blob->sphere.origin = Vec3F(matrix * Vec4F(blob->origin, 1.f));
        blob->sphere.radius = blob->radius;// * matrix[0][0];
    }

    for (HurtBlob* blob : mEnabledHurtBlobs)
    {
        const Mat4F matrix = blob->fighter->get_bone_matrix(blob->bone);

        blob->capsule.originA = Vec3F(matrix * Vec4F(blob->originA, 1.f));
        blob->capsule.originB = Vec3F(matrix * Vec4F(blob->originB, 1.f));
        blob->capsule.radius = blob->radius;// * matrix[0][0];
    }


    //-- find all collisions between hit and hurt blobs ------//

    for (HitBlob* hit : mEnabledHitBlobs)
    {
        for (HurtBlob* hurt : mEnabledHurtBlobs)
        {
            // check if both blobs belong to the same fighter
            if (&hit->action->fighter == hurt->fighter) continue;

            // check if the blobs are not intersecting
            if (maths::intersect_sphere_capsule(hit->sphere, hurt->capsule) == -1) continue;

            // check if the group has already hit the fighter
            if (mHitBlobGroups[hit->action->fighter.index][hit->group][hurt->fighter->index]) continue;

            // add the collision to the appropriate vector
            mCollisions[hurt->fighter->index].push_back({*hit, *hurt});
        }
    }

    //--------------------------------------------------------//

    // todo: hitblobs can hit each other and cause a clash (rebound) effect

    //--------------------------------------------------------//

    constexpr const auto pick_best = [](Collision* champ, Collision* contender) -> Collision*
    {
        if (champ == nullptr) return contender;
        if (champ->hit.index < contender->hit.index) return champ;
        if (champ->hurt.region < contender->hurt.region) return champ;
        return contender;
    };

    std::array<Collision*, MAX_FIGHTERS> bestCollisions {};

    for (size_t hurtFighter = 0u; hurtFighter < MAX_FIGHTERS; ++hurtFighter)
        for (Collision& collision : mCollisions[hurtFighter])
            bestCollisions[hurtFighter] = pick_best(bestCollisions[hurtFighter], &collision);

    for (Collision* champ : bestCollisions)
    {
        if (champ == nullptr) continue;
        mHitBlobGroups[champ->hit.action->fighter.index][champ->hit.group][champ->hurt.fighter->index] = true;
        champ->hurt.fighter->apply_hit_generic(champ->hit, champ->hurt);
    }

    // todo: maybe remove only the best collisions, leave others for next frame
    for (auto& vector : mCollisions)
        vector.clear();
}

//============================================================================//

void FightWorld::set_stage(std::unique_ptr<Stage> stage)
{
    mStage = std::move(stage);
}

void FightWorld::add_fighter(std::unique_ptr<Fighter> fighter)
{
    SQASSERT(mFighters[fighter->index] == nullptr, "index already added");

    const auto iter = algo::find_if(mFighterRefs, [&](auto& it) { return it->index > fighter->index; });
    mFighterRefs.insert(iter, fighter.get());

    mFighters[fighter->index] = std::move(fighter);
}

//============================================================================//

void FightWorld::enable_hitblob(HitBlob* blob)
{
    auto iter = mEnabledHitBlobs.end();
    while (iter != mEnabledHitBlobs.begin())
    {
        if (*std::prev(iter) == blob) return;
        if (*std::prev(iter) < blob) break;
        iter = std::prev(iter);
    }

    mEnabledHitBlobs.insert(iter, blob);
}

void FightWorld::disable_hitblob(HitBlob* blob)
{
    const auto iter = algo::find(mEnabledHitBlobs, blob);
    if (iter == mEnabledHitBlobs.end()) return;

    mEnabledHitBlobs.erase(iter);
}

void FightWorld::disable_hitblobs(const Action& action)
{
    const auto predicate = [&](HitBlob* blob) { return blob->action == &action; };
    algo::erase_if(mEnabledHitBlobs, predicate);
}

void FightWorld::editor_clear_hitblobs()
{
    mEnabledHitBlobs.clear();
}

//============================================================================//

void FightWorld::enable_hurtblob(HurtBlob* blob)
{
    auto iter = mEnabledHurtBlobs.end();
    while (iter != mEnabledHurtBlobs.begin())
    {
        if (*std::prev(iter) == blob) return;
        if (*std::prev(iter) < blob) break;
        iter = std::prev(iter);
    }

    mEnabledHurtBlobs.insert(iter, blob);
}

void FightWorld::disable_hurtblob(HurtBlob* blob)
{
    const auto iter = algo::find(mEnabledHurtBlobs, blob);
    if (iter == mEnabledHurtBlobs.end()) return;

    mEnabledHurtBlobs.erase(iter);
}

void FightWorld::disable_hurtblobs(const Fighter& fighter)
{
    const auto predicate = [&](HurtBlob* blob) { return blob->fighter == &fighter; };
    algo::erase_if(mEnabledHurtBlobs, predicate);
}

void FightWorld::editor_clear_hurtblobs()
{
    mEnabledHurtBlobs.clear();
}

//============================================================================//

void FightWorld::reset_collisions(uint8_t fighter, uint8_t group)
{
    mHitBlobGroups[fighter][group].fill(false);
}

void FightWorld::reset_all_collisions(uint8_t fighter)
{
    for (auto& group : mHitBlobGroups[fighter])
        group.fill(false);
}
