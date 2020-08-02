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
    actionType.set_function("finish_action", &Action::lua_func_finish_action);
    actionType.set_function("enable_blob", &Action::lua_func_enable_blob);
    actionType.set_function("disable_blob", &Action::lua_func_disable_blob);
    actionType.set_function("emit_particles", &Action::lua_func_emit_particles);

    sol::usertype<Fighter> fighterType = lua.new_usertype<Fighter>("Fighter", sol::no_constructor);

    fighterType.set("intangible", sol::property(&Fighter::lua_get_intangible, &Fighter::lua_set_intangible));
    fighterType.set("velocity_x", sol::property(&Fighter::lua_get_velocity_x, &Fighter::lua_set_velocity_x));
    fighterType.set("velocity_y", sol::property(&Fighter::lua_get_velocity_y, &Fighter::lua_set_velocity_y));

    fighterType.set("facing", sol::readonly_property(&Fighter::lua_get_facing));
}

FightWorld::~FightWorld() = default;

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
    //-- misc helper lambda functions ------------------------//

    const auto check_hit_bit = [this](HitBlob* hit, HurtBlob* hurt) -> bool
    {
        uint32_t bits = mHitBitsArray[hit->fighter->index][hurt->fighter->index];
        return (bits & (1u << hit->group)) != 0u;
    };

    const auto fill_hit_bit = [this](HitBlob* hit, HurtBlob* hurt) -> void
    {
        uint32_t& bits = mHitBitsArray[hit->fighter->index][hurt->fighter->index];
        bits = (bits | (1u << hit->group));
    };

    //-- update world space shapes of hit and hurt blobs -----//

    for (HitBlob* blob : mEnabledHitBlobs)
    {
        Mat4F matrix = blob->fighter->get_model_matrix();

        if (blob->bone >= 0)
        {
            const auto& matrices = blob->fighter->get_bone_matrices();
            const auto& boneMatrix = matrices[uint(blob->bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

        blob->sphere.origin = Vec3F(matrix * Vec4F(blob->origin, 1.f));
        blob->sphere.radius = blob->radius;// * matrix[0][0];
    }

    for (HurtBlob* blob : mEnabledHurtBlobs)
    {
        Mat4F matrix = blob->fighter->get_model_matrix();

        if (blob->bone >= 0)
        {
            const auto& matrices = blob->fighter->get_bone_matrices();
            const auto& boneMatrix = matrices[uint(blob->bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

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
            if (check_hit_bit(hit, hurt)) continue;

            // add the collision to the appropriate vector
            mCollisions[hit->fighter->index][hurt->fighter->index].push_back({hit, hurt});
        }
    }

    //--------------------------------------------------------//

    // the rest of this very bad, but somehow it works
    // todo: make less suck

    struct FinalResult
    {
        HitBlob* blob;
        Fighter& other;
    };

    std::vector<FinalResult> finalResultVec;

    std::array<HitBlob*, 32> bestHitBlobs {};
    std::array<HurtBlob*, 32> bestHurtBlobs {};

    for (auto& collisionsArr : mCollisions)
    {
        for (auto& collisionsVec : collisionsArr)
        {
            for (auto [hit, hurt] : collisionsVec)
            {
                const uint8_t group = hit->group;

                if ( bestHitBlobs[group] == nullptr ||
                     hit->damage > bestHitBlobs[group]->damage )
                {
                    bestHitBlobs[group] = hit;
                    bestHurtBlobs[group] = hurt;
                }
            }

            for (uint i = 0u; i < 32u; ++i)
            {
                if (HitBlob* hitBlob = bestHitBlobs[i])
                {
                    finalResultVec.push_back({ hitBlob, *bestHurtBlobs[i]->fighter });

                    fill_hit_bit(bestHitBlobs[i], bestHurtBlobs[i]);
                }
            }

            bestHitBlobs.fill(nullptr);
            bestHurtBlobs.fill(nullptr);

            collisionsVec.clear();
        }
    }

    for (auto& [blob, other] : finalResultVec)
    {
        //blob->action->on_collide(blob, other);
        other.apply_hit_basic(*blob);
    }
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
    mHitBitsArray[fighter.index].fill(uint32_t(0u));
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
