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
WRENPLUS_TRAITS_DEFINITION(sts::Diamond, "Physics", "Diamond")
WRENPLUS_TRAITS_DEFINITION(sts::Ledge, "Stage", "Ledge")
WRENPLUS_TRAITS_DEFINITION(sts::Stage, "Stage", "Stage")
WRENPLUS_TRAITS_DEFINITION(sts::World, "World", "World")

WRENPLUS_BASE_CLASS_DEFINITION(sts::Entity, sts::Article, sts::Fighter)

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
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, pressGrab, "pressGrab");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdAttack, "holdAttack");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdSpecial, "holdSpecial");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdJump, "holdJump");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdShield, "holdShield");
    WRENPLUS_ADD_FIELD_R(vm, InputFrame, holdGrab, "holdGrab");
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
    WRENPLUS_ADD_FIELD_RW(vm, Article::Variables, bully, "bully");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, bully, "bully");
    WRENPLUS_ADD_FIELD_RW(vm, Article::Variables, victim, "victim");
    WRENPLUS_ADD_FIELD_RW(vm, Fighter::Variables, victim, "victim");

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
    WRENPLUS_ADD_FIELD_R(vm, Fighter, variables, "variables");
    WRENPLUS_ADD_FIELD_R(vm, Fighter, diamond, "diamond");
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
    WRENPLUS_ADD_METHOD(vm, FighterAction, wren_throw_victim, "throw_victim(_)");

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

    // Diamond
    WRENPLUS_ADD_FIELD_R(vm, Diamond, cross, "cross");
    WRENPLUS_ADD_FIELD_R(vm, Diamond, min, "min");
    WRENPLUS_ADD_FIELD_R(vm, Diamond, max, "max");

    vm.load_module("Physics");
    vm.cache_handles<Diamond>();

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

    for (auto& fighter : get_sorted_fighters())
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

Stage& World::create_stage(TinyString name)
{
    SQASSERT(mStage == nullptr, "stage already created");

    // for now, stages don't have a def
    mStage = std::make_unique<Stage>(*this, name);
    return *mStage;
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
        mFighters[0]->set_spawn_transform({0.f, 0.f}, +1);
    }
    else if (mFighters.size() == 2u)
    {
        mFighters[0]->set_spawn_transform({-2.f, 0.f}, +1);
        mFighters[1]->set_spawn_transform({+2.f, 0.f}, -1);
    }
    else if (mFighters.size() == 3u)
    {
        mFighters[0]->set_spawn_transform({-2.f, 0.f}, +1);
        mFighters[1]->set_spawn_transform({ 0.f, 0.f}, +1);
        mFighters[2]->set_spawn_transform({+2.f, 0.f}, -1);
    }
    else if (mFighters.size() == 4u)
    {
        mFighters[0]->set_spawn_transform({-6.f, 0.f}, +1);
        mFighters[1]->set_spawn_transform({-2.f, 0.f}, +1);
        mFighters[2]->set_spawn_transform({+2.f, 0.f}, -1);
        mFighters[3]->set_spawn_transform({+6.f, 0.f}, -1);
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
            const Mat4F matrix = fighter->get_model_matrix(blob.def.bone);

            blob.capsule.originA = Vec3F(matrix * Vec4F(blob.def.originA, 1.f));
            blob.capsule.originB = Vec3F(matrix * Vec4F(blob.def.originB, 1.f));
            blob.capsule.radius = blob.def.radius;
        }
    }

    const auto update_hit_blob_transforms = [](Entity& entity)
    {
        for (HitBlob& blob : entity.get_hit_blobs())
        {
            const Mat4F matrix = entity.get_model_matrix(blob.def.bone);

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

    // max damage of hitblobs causing rebound
    std::array<float, MAX_FIGHTERS> reboundDamages {};

    for (size_t firstIndex = 0; firstIndex + 1 < mFighters.size(); ++firstIndex)
    {
        auto& firstFighter = mFighters[firstIndex];
        for (HitBlob& firstBlob : firstFighter->get_hit_blobs())
        {
            for (size_t secondIndex = firstIndex + 1; secondIndex < mFighters.size(); ++secondIndex)
            {
                auto& secondFighter = mFighters[secondIndex];
                for (HitBlob& secondBlob : secondFighter->get_hit_blobs())
                {
                    // blobs do not intersect
                    if (!maths::intersect_capsule_capsule(firstBlob.capsule, secondBlob.capsule)) continue;
\
                    if (firstBlob.def.type == BlobType::Damage)
                    {
                        if (secondBlob.def.type != BlobType::Damage) continue;

                        // first or second blob is transcendent
                        if (firstBlob.def.clangMode == BlobClangMode::Ignore || secondBlob.def.clangMode == BlobClangMode::Ignore) continue;

                        // todo: air attacks can clang with projectiles
                        if (firstBlob.def.clangMode == BlobClangMode::Air || secondBlob.def.clangMode == BlobClangMode::Air) continue;

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

                    else if (firstBlob.def.type == BlobType::Grab)
                    {
                        if (secondBlob.def.type != BlobType::Grab) continue;

                        // grabs can only rebound if both fighters are facing each other
                        const auto &firstVars = firstFighter->variables, &secondVars = secondFighter->variables;
                        if (firstVars.position.x < secondVars.position.x && (firstVars.facing == -1 || secondVars.facing == +1)) continue;
                        if (firstVars.position.x > secondVars.position.x && (firstVars.facing == +1 || secondVars.facing == -1)) continue;

                        // grabs always rebound with the same damage
                        reboundDamages[firstFighter->index] = std::max(reboundDamages[firstFighter->index], 5.f);
                        reboundDamages[secondFighter->index] = std::max(reboundDamages[secondFighter->index], 5.f);
                        firstBlob.cancelled = true;
                        secondBlob.cancelled = true;
                    }

                    else SQEE_UNREACHABLE();
                }
            }
        }
    }

    //-- find collisions between hurt and hit blobs ----------//

    struct HurtCollision { HurtBlob* hurt; HitBlob* hit; };

    using HurtCollisionMap = std::map<Entity*, HurtCollision>;
    using HurtMapItem = HurtCollisionMap::value_type;

    // best collision for each [fighter][entity] combination
    std::array<HurtCollisionMap, MAX_FIGHTERS> hurtCollisionMaps;

    for (auto& hurtFighter : mFighters)
    {
        const Fighter::Variables& hurtVars = hurtFighter->variables;

        if (hurtVars.intangible) continue;

        for (HurtBlob& hurtBlob : hurtFighter->get_hurt_blobs())
        {
            if (hurtBlob.intangible) continue;

            const auto find_hit_blob_collisions = [&](Entity& hitEntity)
            {
                // hitEntity has already hit hurtFighter since the last reset
                if (auto& vec = hitEntity.get_ignore_collisions(); ranges::find(vec, hurtFighter->eid) != vec.end()) return;

                // defer inserting new map entry until there is a collision
                HurtMapItem* mapItem = nullptr;

                for (HitBlob& hitBlob : hitEntity.get_hit_blobs())
                {
                    if (hitBlob.def.type == BlobType::Grab)
                    {
                        const Fighter::Variables& hitVars = static_cast<Fighter&>(hitEntity).variables;

                        // fighter is not grabable
                        if (!hurtVars.grabable) continue;

                        // can only grab fighters in front of you
                        if (hitVars.facing == -1 && hitVars.position.x < hurtVars.position.x) continue;
                        if (hitVars.facing == +1 && hitVars.position.x > hurtVars.position.x) continue;
                    }

                    // hitBlob got cancelled by another hitBlob
                    if (hitBlob.cancelled) continue;

                    // hitBlob can not hit grounded or airborne fighter
                    if (!hitBlob.def.canHitGround && hurtVars.onGround) continue;
                    if (!hitBlob.def.canHitAir && !hurtVars.onGround) continue;

                    // blobs do not intersect
                    if (!maths::intersect_capsule_capsule(hitBlob.capsule, hurtBlob.capsule)) continue;

                    // already have a collision, check if the new one is better
                    if (mapItem != nullptr)
                    {
                        HurtCollision& best = mapItem->second;

                        // todo: should invincible hurtBlobs have special rules?

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

    //-- apply hits and build lists of possible grabs --------//

    // whether each fighter got hit by any attacks
    std::array<bool, MAX_FIGHTERS> hitByAttack {};

    // list of potential grab victims for each fighter
    std::array<StackVector<Fighter*, MAX_FIGHTERS-1>, MAX_FIGHTERS> possibleGrabs;

    for (auto& hurtFighter : mFighters)
    {
        if (hurtCollisionMaps[hurtFighter->index].empty() == false)
        {
            for (const auto& [hitEntity, collision] : hurtCollisionMaps[hurtFighter->index])
            {
                if (collision.hit->def.type == BlobType::Damage)
                {
                    // note that the order that hits accumulate does not matter
                    if (hurtFighter->accumulate_hit(*collision.hit, *collision.hurt))
                        hitByAttack[hurtFighter->index] = true;

                    hitEntity->get_ignore_collisions().push_back(hurtFighter->eid);
                }
                else if (collision.hit->def.type == BlobType::Grab)
                {
                    Fighter* hitFighter = dynamic_cast<Fighter*>(hitEntity);
                    SQASSERT(hitFighter != nullptr, "non-fighter hitblob can't be a grab");

                    // todo: check the stage for a wall between the fighters
                    possibleGrabs[hitFighter->index].push_back(hurtFighter.get());
                }
                else SQEE_UNREACHABLE();
            }

            if (hitByAttack[hurtFighter->index] == true)
            {
                // change action and state after one or more hits are accumlated
                hurtFighter->apply_hits();

                // getting hit by an attack prevents grabbing and rebounding
                possibleGrabs[hurtFighter->index].clear();
                reboundDamages[hurtFighter->index] = 0.f;
            }
        }
    }

    //-- find the closest possible grab for each fighter -----//

    std::array<Fighter*, MAX_FIGHTERS> closestGrabs {};

    for (auto& fighter : mFighters)
    {
        // can only grab fighters that weren't hit by an attack
        sq::erase_if(possibleGrabs[fighter->index], [&](Fighter* victim) { return hitByAttack[victim->index]; });

        if (possibleGrabs[fighter->index].empty() == false)
        {
            // technically port priority if two fighters are somehow EXACTLY the same distance away
            closestGrabs[fighter->index] = *ranges::min_element (
                possibleGrabs[fighter->index], {},
                [&](Fighter* victim) { return maths::distance_squared(fighter->variables.position, victim->variables.position); }
            );
        }
    }

    //-- confirm all non-circular grabs and apply them -------//

    if (ranges::any_of(closestGrabs, std::identity())) // at least one non-nullptr
    {
        // will randomise winners of any contested grabs
        StackVector<Fighter*, MAX_FIGHTERS> shuffled;
        for (auto& fighter : mFighters) shuffled.push_back(fighter.get());
        ranges::shuffle(shuffled, mRandNumGen);

        std::array<bool, MAX_FIGHTERS> confirmedGrabs {};

        while (true)
        {
            bool confirmedSome = false;

            for (auto& fighter : shuffled)
            {
                if (auto& victim = closestGrabs[fighter->index])
                {
                    // nobody else is trying to grab us
                    if (ranges::find(closestGrabs, fighter) == closestGrabs.end())
                    {
                        // prevent anyone else from grabbing our victim
                        for (auto& otherVictim : closestGrabs)
                            if (&otherVictim != &victim && otherVictim == victim)
                                otherVictim = nullptr;

                        // prevent our victim from grabbing anyone else
                        closestGrabs[victim->index] = nullptr;

                        confirmedSome = confirmedGrabs[fighter->index] = true;
                    }
                }
            }

            // all possible grabs have been confirmed
            if (ranges::equal(closestGrabs, confirmedGrabs, [](Fighter* lhs, bool rhs) { return bool(lhs) == rhs; })) break;

            // failed to confirm any grabs, the rest are circular
            if (confirmedSome == false) break;
        }

        for (auto& fighter : mFighters)
        {
            if (auto& victim = closestGrabs[fighter->index])
            {
                if (confirmedGrabs[fighter->index] == true)
                    fighter->apply_grab(*victim);

                // our grab was prevented due to circular grabs
                else reboundDamages[fighter->index] = 5.f;
            }
        }
    }

    //-- apply rebounds --------------------------------------//

    for (auto& fighter : mFighters)
        if (reboundDamages[fighter->index] != 0.f)
            fighter->apply_rebound(reboundDamages[fighter->index]);
}

//============================================================================//

MinMax<Vec2F> World::compute_fighter_bounds() const
{
    MinMax<Vec2F> result;

    for (const auto& fighter : mFighters)
    {
        const auto& vars = fighter->variables;

        // todo: better estimates for fighter model size (or some attributes)
        result.min.x = maths::min(result.min.x, vars.position.x + float(vars.facing) * 0.5f - 1.5f);
        result.min.y = maths::min(result.min.y, vars.position.y - 0.5f);
        result.max.x = maths::max(result.max.x, vars.position.x + float(vars.facing) * 0.5f + 1.5f);
        result.max.y = maths::max(result.max.y, vars.position.y + 2.5f);
    }

    return result;
}

StackVector<Fighter*, MAX_FIGHTERS> World::get_sorted_fighters() const
{
    StackVector<Fighter*, MAX_FIGHTERS> result;

    for (auto& fighter : mFighters)
        // make sure bullies get updated before their victims
        result.insert(ranges::find(result, fighter->variables.victim), fighter.get());

    return result;
}