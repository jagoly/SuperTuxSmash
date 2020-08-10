#include "game/FightWorld.hpp"

#include "main/Options.hpp"

#include "game/Action.hpp"
#include "game/Blobs.hpp"
#include "game/Fighter.hpp"
#include "game/ParticleSystem.hpp"
#include "game/Stage.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Culling.hpp>

using namespace sts;

//============================================================================//

// todo: might want to move the lua setup stuff to a different file

template <class Handler>
bool sol_lua_check(sol::types<TinyString>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
    const int absIndex = sol::absolute_index(L, index);
    const bool success = sol::stack::check<const char*>(L, absIndex, handler);
    tracking.use(1); return success;
}

TinyString sol_lua_get(sol::types<TinyString>, lua_State* L, int index, sol::stack::record& tracking)
{
    const int absIndex = sol::absolute_index(L, index);
    const char* const result = sol::stack::get<const char*>(L, absIndex);
    tracking.use(1); return result;
}

int sol_lua_push(sol::types<TinyString>, lua_State* L, const TinyString& str)
{
    return sol::stack::push(L, str.c_str());
}

//============================================================================//

FightWorld::FightWorld(const Options& _options)
    : options(_options)
    , mHurtBlobAlloc(options.editor_mode ? 128 * 64 : 128)
    , mHitBlobAlloc(options.editor_mode ? 1024 * 64 : 1024)
    , mEmitterAlloc(options.editor_mode ? 1024 * 64 : 1024)
{
    mLuaState = std::make_unique<sol::state>();
    mParticleSystem = std::make_unique<ParticleSystem>();

    sol::state& lua = *mLuaState;

    lua.open_libraries(sol::lib::base, sol::lib::coroutine);

    sol::usertype<Action> actionType = lua.new_usertype<Action>("Action", sol::no_constructor);

    actionType.set_function("wait_until", sol::yielding(&Action::lua_func_wait_until));
    actionType.set_function("allow_interrupt", &Action::lua_func_allow_interrupt);
    //actionType.set_function("enable_blob", &Action::lua_func_enable_blob);
    //actionType.set_function("disable_blob", &Action::lua_func_disable_blob);
    actionType.set_function("enable_blob_group", &Action::lua_func_enable_blob_group);
    actionType.set_function("disable_blob_group", &Action::lua_func_disable_blob_group);
    actionType.set_function("emit_particles", &Action::lua_func_emit_particles);

    sol::usertype<Fighter> fighterType = lua.new_usertype<Fighter>("Fighter", sol::no_constructor);

    fighterType.set("intangible", sol::property(&Fighter::lua_get_intangible, &Fighter::lua_set_intangible));
    fighterType.set("velocity_x", sol::property(&Fighter::lua_get_velocity_x, &Fighter::lua_set_velocity_x));
    fighterType.set("velocity_y", sol::property(&Fighter::lua_get_velocity_y, &Fighter::lua_set_velocity_y));

    fighterType.set("facing", sol::readonly_property(&Fighter::lua_get_facing));
}

FightWorld::~FightWorld() = default;

//============================================================================//

void FightWorld::finish_setup()
{
    if (mFighterRefs.size() == 1u)
    {
        // leave the single fighter where they are
    }
    else if (mFighterRefs.size() == 2u)
    {
        mFighterRefs[0]->current.position.x = -2.f;
        mFighterRefs[0]->previous.position.x = -2.f;
        mFighterRefs[1]->current.position.x = +2.f;
        mFighterRefs[1]->previous.position.x = +2.f;
        mFighterRefs[1]->status.facing = -1;
    }
    else if (mFighterRefs.size() == 3u)
    {
        mFighterRefs[0]->current.position.x = -4.f;
        mFighterRefs[0]->previous.position.x = -4.f;
        mFighterRefs[2]->current.position.x = +4.f;
        mFighterRefs[2]->previous.position.x = +4.f;
        mFighterRefs[2]->status.facing = -1;
    }
    else if (mFighterRefs.size() == 4u)
    {
        mFighterRefs[0]->current.position.x = -6.f;
        mFighterRefs[0]->previous.position.x = -6.f;
        mFighterRefs[1]->current.position.x = -2.f;
        mFighterRefs[1]->previous.position.x = -2.f;
        mFighterRefs[2]->current.position.x = +2.f;
        mFighterRefs[2]->previous.position.x = +2.f;
        mFighterRefs[2]->status.facing = -1;
        mFighterRefs[3]->current.position.x = +6.f;
        mFighterRefs[3]->previous.position.x = +6.f;
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
        const Mat4F matrix = blob->fighter->get_bone_matrix(blob->bone);

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
            if (hit->fighter == hurt->fighter) continue;

            // check if the blobs are not intersecting
            if (maths::intersect_sphere_capsule(hit->sphere, hurt->capsule) == -1) continue;

            // check if the group has already hit the fighter
            if (mHitBlobGroups[hit->fighter->index][hit->group][hurt->fighter->index]) continue;

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
        mHitBlobGroups[champ->hit.fighter->index][champ->hit.group][champ->hurt.fighter->index] = true;
        champ->hurt.fighter->apply_hit_basic(champ->hit, champ->hurt);
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

// for now these can silently fail, they may throw in the future

void FightWorld::enable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(mEnabledHitBlobs, blob);
    if (iter != mEnabledHitBlobs.end()) return;

    mEnabledHitBlobs.push_back(blob);
}

void FightWorld::disable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(mEnabledHitBlobs, blob);
    if (iter == mEnabledHitBlobs.end()) return;

    mEnabledHitBlobs.erase(iter);
}

void FightWorld::enable_hurt_blob(HurtBlob *blob)
{
    const auto iter = algo::find(mEnabledHurtBlobs, blob);
    if (iter != mEnabledHurtBlobs.end()) return;

    mEnabledHurtBlobs.push_back(blob);
}

void FightWorld::disable_hurt_blob(HurtBlob* blob)
{
    const auto iter = algo::find(mEnabledHurtBlobs, blob);
    if (iter == mEnabledHurtBlobs.end()) return;

    mEnabledHurtBlobs.erase(iter);
}

//============================================================================//

void FightWorld::reset_all_hit_blob_groups(Fighter& fighter)
{
    for (auto& array : mHitBlobGroups[fighter.index])
        array.fill(false);
}

void FightWorld::disable_all_hit_blobs(Fighter& fighter)
{
    const auto predicate = [&](HitBlob* blob) { return blob->fighter == &fighter; };
    const auto end = std::remove_if(mEnabledHitBlobs.begin(), mEnabledHitBlobs.end(), predicate);
    mEnabledHitBlobs.erase(end, mEnabledHitBlobs.end());
}

void FightWorld::disable_all_hurt_blobs(Fighter &fighter)
{
    const auto predicate = [&](HurtBlob* blob) { return blob->fighter == &fighter; };
    const auto end = std::remove_if(mEnabledHurtBlobs.begin(), mEnabledHurtBlobs.end(), predicate);
    mEnabledHurtBlobs.erase(end, mEnabledHurtBlobs.end());
}

//============================================================================//

void FightWorld::editor_disable_all_hurtblobs()
{
    mEnabledHurtBlobs.clear();
}
