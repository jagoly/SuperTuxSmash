#include "game/FightWorld.hpp"

#include "main/Options.hpp"

#include "game/Controller.hpp"
#include "game/EffectSystem.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/Physics.hpp"
#include "game/Stage.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Culling.hpp>
#include <sqee/misc/Files.hpp>

using namespace sts;

// todo: move collision stuff to a separate CollisionSystem class

//============================================================================//

WRENPLUS_TRAITS_DEFINITION(sq::coretypes::Vec2I, "Base", "Vec2I")
WRENPLUS_TRAITS_DEFINITION(sq::coretypes::Vec2F, "Base", "Vec2F")
WRENPLUS_TRAITS_DEFINITION(sts::InputFrame, "Controller", "InputFrame")
WRENPLUS_TRAITS_DEFINITION(sts::InputHistory, "Controller", "InputHistory")
WRENPLUS_TRAITS_DEFINITION(sts::Controller, "Controller", "Controller")
WRENPLUS_TRAITS_DEFINITION(sts::Fighter::Attributes, "Fighter", "Attributes")
WRENPLUS_TRAITS_DEFINITION(sts::Fighter::Variables, "Fighter", "Variables")
WRENPLUS_TRAITS_DEFINITION(sts::Fighter, "Fighter", "Fighter")
WRENPLUS_TRAITS_DEFINITION(sts::FighterAction, "FighterAction", "FighterAction")
WRENPLUS_TRAITS_DEFINITION(sts::FighterState, "FighterState", "FighterState")
WRENPLUS_TRAITS_DEFINITION(sts::LocalDiamond, "Physics", "LocalDiamond")
WRENPLUS_TRAITS_DEFINITION(sts::Ledge, "Stage", "Ledge")
WRENPLUS_TRAITS_DEFINITION(sts::Stage, "Stage", "Stage")

//============================================================================//

FightWorld::FightWorld(const Options& options, sq::AudioContext& audio, ResourceCaches& caches, Renderer& renderer)
    : options(options), audio(audio), caches(caches), renderer(renderer)
{
    mEffectSystem = std::make_unique<EffectSystem>(renderer);
    mParticleSystem = std::make_unique<ParticleSystem>();

    vm.set_module_import_dirs({"wren", "assets"});

    //--------------------------------------------------------//

    // Vec2I
    WRENPLUS_ADD_FIELD_RW(vm, Vec2I, x, "x");
    WRENPLUS_ADD_FIELD_RW(vm, Vec2I, y, "y");

    // Vec2F
    WRENPLUS_ADD_FIELD_RW(vm, Vec2F, x, "x");
    WRENPLUS_ADD_FIELD_RW(vm, Vec2F, y, "y");

    vm.load_module("Base");
    vm.cache_handles<Vec2I, Vec2F>();

    //--------------------------------------------------------//

    // InputFrame
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, pressAttack, "pressAttack");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, pressSpecial, "pressSpecial");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, pressJump, "pressJump");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, pressShield, "pressShield");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdAttack, "holdAttack");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdSpecial, "holdSpecial");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdJump, "holdJump");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdShield, "holdShield");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, intX, "intX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, intY, "intY");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, mashX, "mashX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, mashY, "mashY");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, modX, "modX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, modY, "modY");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, relIntX, "relIntX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, relMashX, "relMashX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, relModX, "relModX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, floatX, "floatX");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, floatY, "floatY");

    // InputHistory
    WRENPLUS_ADD_METHOD(vm, InputHistory, wren_iterate, "iterate(_)");
    WRENPLUS_ADD_METHOD(vm, InputHistory, wren_iterator_value, "iteratorValue(_)");

    // Controller
    WRENPLUS_ADD_FIELD_R(vm, Controller, history, "history");
    WRENPLUS_ADD_METHOD(vm, Controller, wren_get_input, "input");
    WRENPLUS_ADD_METHOD(vm, Controller, wren_clear_history, "clear_history()");

    vm.load_module("Controller");
    vm.cache_handles<InputFrame, InputHistory, Controller>();

    //--------------------------------------------------------//

    // Attributes
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, walkSpeed, "walkSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, dashSpeed, "dashSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, airSpeed, "airSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, traction, "traction");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, airMobility, "airMobility");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, airFriction, "airFriction");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, hopHeight, "hopHeight");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, jumpHeight, "jumpHeight");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, airHopHeight, "airHopHeight");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, gravity, "gravity");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, fallSpeed, "fallSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, fastFallSpeed, "fastFallSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, weight, "weight");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, extraJumps, "extraJumps");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, lightLandTime, "lightLandTime");

    // Variables
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, position, "position");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, velocity, "velocity");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, facing, "facing");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, extraJumps, "extraJumps");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, lightLandTime, "lightLandTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, noCatchTime, "noCatchTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, stunTime, "stunTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, freezeTime, "freezeTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, edgeStop, "edgeStop");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, intangible, "intangible");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, fastFall, "fastFall");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, applyGravity, "applyGravity");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, applyFriction, "applyFriction");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, flinch, "flinch");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, onGround, "onGround");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, onPlatform, "onPlatform");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, edge, "edge");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, moveMobility, "moveMobility");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, moveSpeed, "moveSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, damage, "damage");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, shield, "shield");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, launchSpeed, "launchSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, attachPoint, "attachPoint");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, ledge, "ledge");

    // Fighter
    WRENPLUS_ADD_FIELD_R(vm, Fighter, name, "name");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, index, "index");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, attributes, "attributes");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, localDiamond, "localDiamond");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, variables, "variables");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, controller, "controller");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, activeAction, "action");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, activeState, "state");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_get_library, "library");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_log, "log(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_clear_action, "cxx_clear_action()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_assign_action, "cxx_assign_action(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_assign_state, "cxx_assign_state(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_auto, "reverse_facing_auto()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_instant, "reverse_facing_instant()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_slow, "reverse_facing_slow(_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_animated, "reverse_facing_animated(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_attempt_ledge_catch, "attempt_ledge_catch()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_play_animation, "play_animation(_,_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_set_next_animation, "set_next_animation(_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reset_collisions, "reset_collisions()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_enable_hurtblob, "enable_hurtblob(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_disable_hurtblob, "disable_hurtblob(_)");
    vm.register_pointer_comparison_operators<Fighter>();

    vm.load_module("Fighter");
    vm.cache_handles<Fighter::Attributes, Fighter::Variables, Fighter>();

    //--------------------------------------------------------//

    // FighterAction
    WRENPLUS_ADD_FIELD_R(vm, FighterAction, name, "name");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_fighter, "fighter");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_script, "script");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_fiber, "fiber");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_set_fiber, "fiber=(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_log_with_prefix, "log_with_prefix(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_before_start, "cxx_before_start()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_wait_until, "cxx_wait_until(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_wait_for, "cxx_wait_for(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_next_frame, "cxx_next_frame()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_before_cancel, "cxx_before_cancel()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_check_hit_something, "check_hit_something()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_enable_hitblobs, "enable_hitblobs(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_disable_hitblobs, "disable_hitblobs(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_emit_particles, "emit_particles(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_play_effect, "play_effect(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_play_sound, "play_sound(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cancel_sound, "cancel_sound(_)");

    vm.load_module("FighterAction");
    vm.cache_handles<FighterAction>();

    //--------------------------------------------------------//

    // FighterState
    WRENPLUS_ADD_FIELD_R(vm, FighterState, name, "name");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_get_fighter, "fighter");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_get_script, "script");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_log_with_prefix, "log_with_prefix(_)");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_cxx_before_enter, "cxx_before_enter()");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_cxx_before_exit, "cxx_before_exit()");

    vm.load_module("FighterState");
    vm.cache_handles<FighterState>();

    //--------------------------------------------------------//

    // LocalDiamond
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, halfWidth, "halfWidth");
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, offsetCross, "offsetCross");
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, offsetTop, "offsetTop");
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, normLeftDown, "normLeftDown");
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, normLeftUp, "normLeftUp");
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, normRightDown, "normRightDown");
    WRENPLUS_ADD_FIELD_R(vm, LocalDiamond, normRightUp, "normRightUp");

    vm.load_module("Physics");
    vm.cache_handles<LocalDiamond>();

    //--------------------------------------------------------//

    // Ledge
    WRENPLUS_ADD_FIELD_R(vm, Ledge, position, "position");
    WRENPLUS_ADD_FIELD_R(vm, Ledge, direction, "direction");
    WRENPLUS_ADD_FIELD_RW(vm, Ledge, grabber, "grabber");

    vm.load_module("Stage");
    vm.cache_handles<Ledge, Stage>();

    //--------------------------------------------------------//

    handles.new_1 = wrenMakeCallHandle(vm, "new(_)");

    handles.action_do_start = wrenMakeCallHandle(vm, "do_start()");
    handles.action_do_updates = wrenMakeCallHandle(vm, "do_updates()");
    handles.action_do_cancel = wrenMakeCallHandle(vm, "do_cancel()");

    handles.state_do_enter = wrenMakeCallHandle(vm, "do_enter()");
    handles.state_do_updates = wrenMakeCallHandle(vm, "do_updates()");
    handles.state_do_exit = wrenMakeCallHandle(vm, "do_exit()");
}

//============================================================================//

FightWorld::~FightWorld()
{
    wrenReleaseHandle(vm, handles.new_1);

    wrenReleaseHandle(vm, handles.state_do_enter);
    wrenReleaseHandle(vm, handles.state_do_updates);
    wrenReleaseHandle(vm, handles.state_do_exit);

    wrenReleaseHandle(vm, handles.action_do_start);
    wrenReleaseHandle(vm, handles.action_do_updates);
    wrenReleaseHandle(vm, handles.action_do_cancel);
}

//============================================================================//

void FightWorld::tick()
{
    mStage->tick();

    for (auto& fighter : mFighters)
        fighter->tick();

    impl_update_collisions();

    mEffectSystem->tick();

    mParticleSystem->update_and_clean();
}

//============================================================================//

void FightWorld::integrate(float blend)
{
    mStage->integrate(blend);

    for (auto& fighter : mFighters)
        fighter->integrate(blend);

    mEffectSystem->integrate(blend);
}

//============================================================================//

void FightWorld::set_stage(std::unique_ptr<Stage> stage)
{
    mStage = std::move(stage);
}

void FightWorld::add_fighter(std::unique_ptr<Fighter> fighter)
{
    SQASSERT(fighter->index == mFighters.size(), "fighter has incorrect index");
    mFighters.emplace_back(std::move(fighter));
}

//============================================================================//

void FightWorld::finish_setup()
{
    if (mFighters.size() == 1u)
    {
        // leave the single fighter where they are
    }
    else if (mFighters.size() == 2u)
    {
        mFighters[0]->variables.position.x = -2.f;
        mFighters[1]->variables.position.x = +2.f;
        mFighters[1]->variables.facing = -1;
    }
    else if (mFighters.size() == 3u)
    {
        mFighters[0]->variables.position.x = -4.f;
        mFighters[2]->variables.position.x = +4.f;
        mFighters[2]->variables.facing = -1;
    }
    else if (mFighters.size() == 4u)
    {
        mFighters[0]->variables.position.x = -6.f;
        mFighters[1]->variables.position.x = -2.f;
        mFighters[2]->variables.position.x = +2.f;
        mFighters[2]->variables.facing = -1;
        mFighters[3]->variables.position.x = +6.f;
        mFighters[3]->variables.facing = -1;
    }
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

void FightWorld::disable_hitblobs(const FighterAction& action)
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
            const Fighter& hitFighter = hit->action->fighter;
            const Fighter& hurtFighter = *hurt->fighter;

            // blobs belong to the same fighter
            if (&hitFighter == &hurtFighter) continue;

            // blob can not hit the other fighter
            if (!hit->canHitGround && hurtFighter.variables.onGround) continue;
            if (!hit->canHitAir && !hurtFighter.variables.onGround) continue;

            // blobs are not intersecting
            if (maths::intersect_sphere_capsule(hit->sphere, hurt->capsule) == -1) continue;

            // fighter already hurt the other fighter
            if (mIgnoreCollisions[hitFighter.index][hurtFighter.index]) continue;

            // add the collision to the appropriate vector
            mCollisions[hurt->fighter->index].push_back({*hit, *hurt});
        }
    }

    //--------------------------------------------------------//

    // todo: hitblobs can hit each other and cause a clang (rebound) effect

    //--------------------------------------------------------//

    constexpr auto pick_best = [](Collision* champ, Collision* contender) -> Collision*
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
        mIgnoreCollisions[champ->hit.action->fighter.index][champ->hurt.fighter->index] = true;
        champ->hurt.fighter->apply_hit(champ->hit, champ->hurt);
    }

    // todo: maybe remove only the best collisions, leave others for next frame
    for (auto& vector : mCollisions)
        vector.clear();
}

//============================================================================//

void FightWorld::reset_collisions(uint8_t fighter)
{
    mIgnoreCollisions[fighter].fill(false);
}

//============================================================================//

MinMax<Vec2F> FightWorld::compute_fighter_bounds() const
{
    MinMax<Vec2F> result;

    for (const auto& fighter : mFighters)
    {
        // todo: better estimates for fighter model size
        result.min.x = maths::min(result.min.x, fighter->variables.position.x + fighter->localDiamond.min().x - 1.f);
        result.min.y = maths::min(result.min.y, fighter->variables.position.y + fighter->localDiamond.min().y - 0.5f);
        result.max.x = maths::max(result.max.x, fighter->variables.position.x + fighter->localDiamond.max().x + 1.f);
        result.max.y = maths::max(result.max.y, fighter->variables.position.y + fighter->localDiamond.max().y + 0.5f);
    }

    return result;
}
