#include "game/World.hpp"

#include "game/Article.hpp"
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

#include <sqee/maths/Culling.hpp>

using namespace sts;

// todo: move collision stuff to a separate CollisionSystem class

//============================================================================//

WRENPLUS_TRAITS_DEFINITION(sq::coretypes::Vec2I, "Base", "Vec2I")
WRENPLUS_TRAITS_DEFINITION(sq::coretypes::Vec2F, "Base", "Vec2F")
WRENPLUS_TRAITS_DEFINITION(sts::InputFrame, "Controller", "InputFrame")
WRENPLUS_TRAITS_DEFINITION(sts::InputHistory, "Controller", "InputHistory")
WRENPLUS_TRAITS_DEFINITION(sts::Controller, "Controller", "Controller")
WRENPLUS_TRAITS_DEFINITION(sts::Article::Variables, "Article", "Variables")
WRENPLUS_TRAITS_DEFINITION(sts::Article, "Article", "Article")
WRENPLUS_TRAITS_DEFINITION(sts::Fighter::Attributes, "Fighter", "Attributes")
WRENPLUS_TRAITS_DEFINITION(sts::Fighter::Variables, "Fighter", "Variables")
WRENPLUS_TRAITS_DEFINITION(sts::Fighter, "Fighter", "Fighter")
WRENPLUS_TRAITS_DEFINITION(sts::FighterAction, "FighterAction", "FighterAction")
WRENPLUS_TRAITS_DEFINITION(sts::FighterState, "FighterState", "FighterState")
WRENPLUS_TRAITS_DEFINITION(sts::LocalDiamond, "Physics", "LocalDiamond")
WRENPLUS_TRAITS_DEFINITION(sts::Ledge, "Stage", "Ledge")
WRENPLUS_TRAITS_DEFINITION(sts::Stage, "Stage", "Stage")
WRENPLUS_TRAITS_DEFINITION(sts::World, "World", "World")

//============================================================================//

World::World(const Options& options, sq::AudioContext& audio, ResourceCaches& caches, Renderer& renderer)
    : options(options), audio(audio), caches(caches), renderer(renderer)
{
    mEffectSystem = std::make_unique<EffectSystem>(renderer);
    mParticleSystem = std::make_unique<ParticleSystem>(*this);

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

    // Variables
    WRENPLUS_ADD_FIELD_R(vm, Article::Variables, position, "position");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, position, "position");
    WRENPLUS_ADD_FIELD_R(vm, Article::Variables, velocity, "velocity");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, velocity, "velocity");
    WRENPLUS_ADD_FIELD_RW(vm, Article::Variables, facing, "facing");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, facing, "facing");
    WRENPLUS_ADD_FIELD_R(vm, Article::Variables, freezeTime, "freezeTime");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, freezeTime, "freezeTime");
    WRENPLUS_ADD_FIELD_RW(vm, Article::Variables, animTime, "animTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, animTime, "animTime");
    WRENPLUS_ADD_FIELD_R(vm, Article::Variables, attachPoint, "attachPoint");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, attachPoint, "attachPoint");
    WRENPLUS_ADD_FIELD_R(vm, Article::Variables, hitSomething, "hitSomething");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, hitSomething, "hitSomething");

    // Entity
    WRENPLUS_ADD_METHOD(vm, Article, wren_get_name, "name");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_get_name, "name");
    WRENPLUS_ADD_METHOD(vm, Article, wren_get_world, "world");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_get_world, "world");
    WRENPLUS_ADD_METHOD(vm, Article, wren_reverse_facing_auto, "reverse_facing_auto()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_auto, "reverse_facing_auto()");
    WRENPLUS_ADD_METHOD(vm, Article, wren_reverse_facing_instant, "reverse_facing_instant()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_instant, "reverse_facing_instant()");
    WRENPLUS_ADD_METHOD(vm, Article, wren_reverse_facing_slow, "reverse_facing_slow(_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_slow, "reverse_facing_slow(_,_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_reverse_facing_animated, "reverse_facing_animated(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reverse_facing_animated, "reverse_facing_animated(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_reset_collisions, "reset_collisions()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_reset_collisions, "reset_collisions()");
    WRENPLUS_ADD_METHOD(vm, Article, wren_play_animation, "play_animation(_,_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_play_animation, "play_animation(_,_,_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_set_next_animation, "set_next_animation(_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_set_next_animation, "set_next_animation(_,_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_play_sound, "play_sound(_,_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_play_sound, "play_sound(_,_)");

    //--------------------------------------------------------//

    // Variables
    WRENPLUS_ADD_FIELD_RW(vm, Article::Variables, fragile, "fragile");
    WRENPLUS_ADD_FIELD_R(vm, Article::Variables, bounced, "bounced");

    // Article
    WRENPLUS_ADD_FIELD_R(vm, Article, fighter, "fighter");
    WRENPLUS_ADD_FIELD_R(vm, Article, variables, "variables");
    WRENPLUS_ADD_METHOD(vm, Article, wren_get_script_class, "scriptClass");
    WRENPLUS_ADD_METHOD(vm, Article, wren_get_script, "script");
    WRENPLUS_ADD_METHOD(vm, Article, wren_set_script, "script=(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_get_fiber, "fiber");
    WRENPLUS_ADD_METHOD(vm, Article, wren_set_fiber, "fiber=(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_log_with_prefix, "log_with_prefix(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_cxx_wait_until, "cxx_wait_until(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_cxx_wait_for, "cxx_wait_for(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_cxx_next_frame, "cxx_next_frame()");
    WRENPLUS_ADD_METHOD(vm, Article, wren_mark_for_destroy, "mark_for_destroy()");
    WRENPLUS_ADD_METHOD(vm, Article, wren_enable_hitblobs, "enable_hitblobs(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_disable_hitblobs, "disable_hitblobs(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_play_effect, "play_effect(_)");
    WRENPLUS_ADD_METHOD(vm, Article, wren_emit_particles, "emit_particles(_)");

    vm.load_module("Article");
    vm.cache_handles<Article::Variables, Article>();

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
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, walkAnimSpeed, "walkAnimSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, dashAnimSpeed, "dashAnimSpeed");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, extraJumps, "extraJumps");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Attributes, lightLandTime, "lightLandTime");

    // Variables
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, extraJumps, "extraJumps");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, lightLandTime, "lightLandTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, noCatchTime, "noCatchTime");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, stunTime, "stunTime");
    WRENPLUS_ADD_FIELD_R(vm, Fighter::Variables, reboundTime, "reboundTime");
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
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, ledge, "ledge");

    // Fighter
    WRENPLUS_ADD_FIELD_R(vm, Fighter, index, "index");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, attributes, "attributes");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, localDiamond, "localDiamond");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, variables, "variables");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, controller, "controller");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, activeAction, "action");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, activeState, "state");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_get_library, "library");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_log, "log(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_assign_action, "cxx_assign_action(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_assign_action_null, "cxx_assign_action_null()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_assign_state, "cxx_assign_state(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_cxx_spawn_article, "cxx_spawn_article(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_attempt_ledge_catch, "attempt_ledge_catch()");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_enable_hurtblob, "enable_hurtblob(_)");
    WRENPLUS_ADD_METHOD(vm, Fighter, wren_disable_hurtblob, "disable_hurtblob(_)");
    vm.register_pointer_comparison_operators<Fighter>();

    vm.load_module("Fighter");
    vm.cache_handles<Fighter::Attributes, Fighter::Variables, Fighter>();

    //--------------------------------------------------------//

    // FighterAction
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_name, "name");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_fighter, "fighter");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_world, "world");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_script, "script");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_get_fiber, "fiber");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_set_fiber, "fiber=(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_log_with_prefix, "log_with_prefix(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_before_start, "cxx_before_start()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_wait_until, "cxx_wait_until(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_wait_for, "cxx_wait_for(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_next_frame, "cxx_next_frame()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_cxx_before_cancel, "cxx_before_cancel()");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_enable_hitblobs, "enable_hitblobs(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_disable_hitblobs, "disable_hitblobs(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_play_effect, "play_effect(_)");
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_emit_particles, "emit_particles(_)");

    vm.load_module("FighterAction");
    vm.cache_handles<FighterAction>();

    //--------------------------------------------------------//

    // FighterState
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_get_name, "name");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_get_fighter, "fighter");
    WRENPLUS_ADD_METHOD(vm, FighterState, wren_get_world, "world");
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

    // World
    WRENPLUS_ADD_METHOD(vm, World, wren_random_int, "random_int(_,_)");
    WRENPLUS_ADD_METHOD(vm, World, wren_random_float, "random_float(_,_)");
    WRENPLUS_ADD_METHOD(vm, World, wren_cancel_sound, "cancel_sound(_)");
    WRENPLUS_ADD_METHOD(vm, World, wren_cancel_effect, "cancel_effect(_)");

    vm.load_module("World");
    vm.cache_handles<World>();

    //--------------------------------------------------------//

    handles.new_1 = wrenMakeCallHandle(vm, "new(_)");

    handles.action_do_start = wrenMakeCallHandle(vm, "do_start()");
    handles.action_do_updates = wrenMakeCallHandle(vm, "do_updates()");
    handles.action_do_cancel = wrenMakeCallHandle(vm, "do_cancel()");

    handles.state_do_enter = wrenMakeCallHandle(vm, "do_enter()");
    handles.state_do_updates = wrenMakeCallHandle(vm, "do_updates()");
    handles.state_do_exit = wrenMakeCallHandle(vm, "do_exit()");

    handles.article_do_updates = wrenMakeCallHandle(vm, "do_updates()");
    handles.article_do_destroy = wrenMakeCallHandle(vm, "do_destroy()");
}

//============================================================================//

World::~World()
{
    wrenReleaseHandle(vm, handles.new_1);

    wrenReleaseHandle(vm, handles.action_do_start);
    wrenReleaseHandle(vm, handles.action_do_updates);
    wrenReleaseHandle(vm, handles.action_do_cancel);

    wrenReleaseHandle(vm, handles.state_do_enter);
    wrenReleaseHandle(vm, handles.state_do_updates);
    wrenReleaseHandle(vm, handles.state_do_exit);

    wrenReleaseHandle(vm, handles.article_do_updates);
    wrenReleaseHandle(vm, handles.article_do_destroy);
}

//============================================================================//

void World::tick()
{
    mStage->tick();

    for (auto& fighter : mFighters)
        fighter->tick();

    for (auto& article : mArticles)
        article->tick();

    impl_update_collisions();

    mEffectSystem->tick();

    mParticleSystem->update_and_clean();

    for (auto iter = mArticles.begin(); iter != mArticles.end();)
    {
        Article& article = **iter;
        if (article.check_marked_for_destroy() == false) ++iter;
        else iter = mArticles.erase(iter);
    }
}

//============================================================================//

void World::integrate(float blend)
{
    mStage->integrate(blend);

    for (auto& fighter : mFighters)
        fighter->integrate(blend);

    for (auto& article : mArticles)
        article->integrate(blend);

    mEffectSystem->integrate(blend);
}

//============================================================================//

void World::set_stage(std::unique_ptr<Stage> stage)
{
    mStage = std::move(stage);
}

Fighter& World::create_fighter(TinyString name)
{
    // find an existing definition or load a new one
    FighterDef& def = mFighterDefs.try_emplace(name, *this, name).first->second;

    const uint8_t index = uint8_t(mFighters.size());
    return *mFighters.emplace_back(std::make_unique<Fighter>(def, index));
}

ArticleDef& World::load_article_def(const String& path)
{
    const auto [iter, created] = mArticleDefs.try_emplace(path, *this, path);

    if (created == true)
    {
        try {
            iter->second.load_json_from_file();
            iter->second.load_wren_from_file();
        }
        catch (const std::exception& ex) {
            sq::log_warning("'{}': {}", path, ex.what());
        }
    }

    return iter->second;
}

Article& World::create_article(const ArticleDef& def, Fighter* fighter)
{
    return *mArticles.emplace_back(std::make_unique<Article>(def, fighter));
}

//============================================================================//

void World::finish_setup()
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

void World::impl_update_collisions()
{
    //-- update capsules of hurt and hit blobs ---------------//

    for (auto& fighter : mFighters)
    {
        for (HurtBlob& blob : fighter->get_hurt_blobs())
        {
            const Mat4F matrix = fighter->get_bone_matrix(blob.def.bone);

            blob.capsule.originA = Vec3F(matrix * Vec4F(blob.def.originA, 1.f));
            blob.capsule.originB = Vec3F(matrix * Vec4F(blob.def.originB, 1.f));
            blob.capsule.radius = blob.def.radius;
        }
    }

    const auto update_hit_blob_transforms = [](Entity& entity)
    {
        for (HitBlob& blob : entity.get_hit_blobs())
        {
            const Mat4F matrix = entity.get_bone_matrix(blob.def.bone);

            if (blob.justCreated == true)
            {
                blob.capsule.originA = Vec3F(matrix * Vec4F(blob.def.origin, 1.f));
                blob.capsule.originB = blob.capsule.originA;
                blob.capsule.radius = blob.def.radius;
                blob.justCreated = false;
            }
            else
            {
                blob.capsule.originB = blob.capsule.originA;
                blob.capsule.originA = Vec3F(matrix * Vec4F(blob.def.origin, 1.f));
            }
        }
    };

    for (auto& fighter : mFighters)
        update_hit_blob_transforms(*fighter);

    for (auto& article : mArticles)
        update_hit_blob_transforms(*article);

    //-- find collisions between hit blobs -------------------//

    struct HitCollision { HitBlob* first; HitBlob* second; };

    using HitCollisionMap = std::map<int32_t, HitCollision>;
    using HitCollisionMapMap = std::map<int32_t, HitCollisionMap>;

    // best collision for each [entity][entity] combination
    HitCollisionMapMap hitCollisionMapMap;

    // max damage of hitboxes causing rebound
    std::array<float, MAX_FIGHTERS> reboundDamages {};

    for (auto& firstFighter : mFighters)
    {
        for (HitBlob& firstBlob : firstFighter->get_hit_blobs())
        {
            // first blob is transcendent
            if (firstBlob.def.clangMode == BlobClangMode::Ignore) continue;

            for (auto& secondFighter : mFighters)
            {
                if (firstFighter == secondFighter) continue;

                for (HitBlob& secondBlob : secondFighter->get_hit_blobs())
                {
                    // second blob is transcendent
                    if (secondBlob.def.clangMode == BlobClangMode::Ignore) continue;

                    // blobs do not intersect
                    if (!maths::intersect_capsule_capsule(firstBlob.capsule, secondBlob.capsule)) continue;

                    if (firstBlob.def.clangMode != BlobClangMode::Air && secondBlob.def.clangMode != BlobClangMode::Air)
                    {
                        const float damageDiff = firstBlob.def.damage - secondBlob.def.damage;

                        if (damageDiff < +9.f) firstBlob.cancelled = true;
                        if (damageDiff > -9.f) secondBlob.cancelled = true;

                        if (damageDiff > -9.f && damageDiff < +9.f)
                        {
                            if (firstBlob.def.clangMode == BlobClangMode::Ground)
                                reboundDamages[firstFighter->index] =
                                    std::max(reboundDamages[firstFighter->index], firstBlob.def.damage);

                            if (secondBlob.def.clangMode == BlobClangMode::Ground)
                                reboundDamages[secondFighter->index] =
                                    std::max(reboundDamages[secondFighter->index], secondBlob.def.damage);
                        }
                    }
                }
            }
        }
    }

    //-- find collisions with fighter hurt blobs -------------//

    struct HurtCollision { HurtBlob* hurt; HitBlob* hit; };

    using HurtCollisionMap = std::map<Entity*, HurtCollision>;
    using HurtMapItem = HurtCollisionMap::value_type;

    // best collision for each [fighter][entity] combination
    std::array<HurtCollisionMap, MAX_FIGHTERS> hurtCollisionMaps;

    for (auto& hurtFighter : mFighters)
    {
        Fighter::Variables& hurtVars = hurtFighter->variables;

        if (hurtVars.intangible == true) continue;

        for (HurtBlob& hurtBlob : hurtFighter->get_hurt_blobs())
        {
            if (hurtBlob.intangible == true) continue;

            const auto find_hit_blob_collisions = [&](Entity& hitEntity)
            {
                // hitEntity has already hit hurtFighter since the last reset
                if (auto& vec = hitEntity.get_ignore_collisions(); ranges::find(vec, hurtFighter->eid) != vec.end()) return;

                // defer inserting new map entry until there is a collision
                HurtMapItem* mapItem = nullptr;

                for (HitBlob& hitBlob : hitEntity.get_hit_blobs())
                {
                    // hitBlob got cancelled by another hitBlob
                    if (hitBlob.cancelled == true) continue;

                    // hitBlob can not hit hurtFighter
                    if (!hitBlob.def.canHitGround && hurtVars.onGround) continue;
                    if (!hitBlob.def.canHitAir && !hurtVars.onGround) continue;

                    // blobs do not intersect
                    if (!maths::intersect_capsule_capsule(hitBlob.capsule, hurtBlob.capsule)) continue;

                    // already have a collision, check if the new one is better
                    if (mapItem != nullptr)
                    {
                        HurtCollision& best = mapItem->second;

                        // same hitBlob, choose hurtBlob with highest priority
                        if (&hitBlob == best.hit)
                        {
                            // lower enum value means higher priority (middle, lower, upper)
                            if (hurtBlob.def.region >= best.hurt->def.region) continue;
                        }

                        // choose hitBlob with highest priority
                        else if (hitBlob.def.index >= best.hit->def.index) continue;
                    }

                    // first time this frame that hurtFighter was hit by hitEntity
                    else mapItem = &*hurtCollisionMaps[hurtBlob.fighter.index].try_emplace(&hitEntity).first;

                    mapItem->second = { &hurtBlob, &hitBlob };
                }
            };

            for (auto& hitFighter : mFighters)
                if (hitFighter != hurtFighter)
                    find_hit_blob_collisions(*hitFighter);

            for (auto& hitArticle : mArticles)
                if (hitArticle->fighter != hurtFighter.get())
                    find_hit_blob_collisions(*hitArticle);
        }
    }

    //--------------------------------------------------------//

    for (size_t fighterIndex = 0u; fighterIndex < mFighters.size(); ++fighterIndex)
    {
        Fighter& fighter = *mFighters[fighterIndex];
        const auto& collisionMap = hurtCollisionMaps[fighterIndex];

        if (collisionMap.empty() == false)
        {
            for (const auto& [hitEntity, collision] : collisionMap)
            {
                fighter.accumulate_hit(*collision.hit, *collision.hurt);
                hitEntity->get_ignore_collisions().push_back(collision.hurt->fighter.eid);
            }
            fighter.apply_hits();
        }

        else if (reboundDamages[fighterIndex] != 0.f)
            fighter.apply_rebound(reboundDamages[fighterIndex]);
    }
}

//============================================================================//

MinMax<Vec2F> World::compute_fighter_bounds() const
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
