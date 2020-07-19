#include "game/FightWorld.hpp"

#include "game/Actions.hpp"
#include "game/Fighter.hpp"
#include "game/Stage.hpp"
#include "game/private/PrivateWorld.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Algorithms.hpp>

namespace algo = sq::algo;
namespace maths = sq::maths;

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

FightWorld::FightWorld(const Globals& globals)
    : globals(globals)
    , mHurtBlobAlloc(globals.editorMode ? 128 * 64 : 128)
    , mHitBlobAlloc(globals.editorMode ? 1024 * 64 : 1024)
    , mEmitterAlloc(globals.editorMode ? 1024 * 64 : 1024)
{
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

    impl = std::make_unique<PrivateWorld>(*this);
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

    impl->tick();

    for (auto& fighter : mFighters)
    {
        if (fighter != nullptr)
            mStage->check_boundary(*fighter);
    }

    mParticleSystem.update_and_clean();
}

//============================================================================//

void FightWorld::set_stage(UniquePtr<Stage> stage)
{
    mStage = std::move(stage);
}

void FightWorld::add_fighter(UniquePtr<Fighter> fighter)
{
    mFighters[fighter->index] = std::move(fighter);
}

//============================================================================//

const Vector<HitBlob*>& FightWorld::get_hit_blobs() const
{
    return impl->enabledHitBlobs;
}

const Vector<HurtBlob*>& FightWorld::get_hurt_blobs() const
{
    return impl->enabledHurtBlobs;
}

//============================================================================//

// for now these can silently fail, they may throw in the future

void FightWorld::enable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(impl->enabledHitBlobs, blob);
    if (iter != impl->enabledHitBlobs.end()) return;

    impl->enabledHitBlobs.push_back(blob);
}

void FightWorld::disable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(impl->enabledHitBlobs, blob);
    if (iter == impl->enabledHitBlobs.end()) return;

    impl->enabledHitBlobs.erase(iter);
}

void FightWorld::enable_hurt_blob(HurtBlob *blob)
{
    const auto iter = algo::find(impl->enabledHurtBlobs, blob);
    if (iter != impl->enabledHurtBlobs.end()) return;

    impl->enabledHurtBlobs.push_back(blob);
}

void FightWorld::disable_hurt_blob(HurtBlob* blob)
{
    const auto iter = algo::find(impl->enabledHurtBlobs, blob);
    if (iter == impl->enabledHurtBlobs.end()) return;

    impl->enabledHurtBlobs.erase(iter);
}

//============================================================================//

void FightWorld::reset_all_hit_blob_groups(Fighter& fighter)
{
    impl->hitBitsArray[fighter.index].fill(uint32_t(0u));
}

void FightWorld::disable_all_hit_blobs(Fighter& fighter)
{
    const auto predicate = [&](HitBlob* blob) { return blob->fighter == &fighter; };
    const auto end = std::remove_if(impl->enabledHitBlobs.begin(), impl->enabledHitBlobs.end(), predicate);
    impl->enabledHitBlobs.erase(end, impl->enabledHitBlobs.end());
}

void FightWorld::disable_all_hurt_blobs(Fighter &fighter)
{
    const auto predicate = [&](HurtBlob* blob) { return blob->fighter == &fighter; };
    const auto end = std::remove_if(impl->enabledHurtBlobs.begin(), impl->enabledHurtBlobs.end(), predicate);
    impl->enabledHurtBlobs.erase(end, impl->enabledHurtBlobs.end());
}

//============================================================================//

SceneData FightWorld::compute_scene_data() const
{
    SceneData result;

    result.view = { Vec2F(+INFINITY), Vec2F(-INFINITY) };

    for (const auto& fighter : mFighters)
    {
        if (fighter == nullptr) continue;

        const Vec2F centre = fighter->get_position() + fighter->diamond.cross();

        result.view.min = maths::min(result.view.min, centre);
        result.view.max = maths::max(result.view.max, centre);
    }

    result.inner.min = mStage->get_inner_boundary().min;
    result.inner.max = mStage->get_inner_boundary().max;

    result.outer.min = mStage->get_outer_boundary().min;
    result.outer.max = mStage->get_outer_boundary().max;

    return result;
}

//============================================================================//

void FightWorld::editor_disable_all_hurtblobs()
{
    impl->enabledHurtBlobs.clear();
}
