#include "editor/Editor_Action.hpp"

#include "main/SmashApp.hpp"

#include "game/EffectSystem.hpp"
#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HitBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

#include "editor/Editor_Helpers.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;
using ActionContext = EditorScene::ActionContext;

//============================================================================//

ActionContext::ActionContext(EditorScene& _editor, ActionKey _key)
    : BaseContext(_editor, StageEnum::TestZone), ctxKey(_key)
{
    ctxTypeString = "Action";
    ctxKeyString = "{}/{}"_format(ctxKey.fighter, ctxKey.name);

    world->add_fighter(std::make_unique<Fighter>(*world, ctxKey.fighter, 0u));
    world->get_fighter(0u).controller = editor.mController.get();

    fighter = &world->get_fighter(0u);
    action = &fighter->mActions.at(ctxKey.name);

    savedData = action->clone();
    undoStack.push_back(action->clone());

    reset_timeline_length();
    scrub_to_frame(-1);
}

ActionContext::~ActionContext() = default;

//============================================================================//

void ActionContext::apply_working_changes()
{
    if (action->has_changes(*undoStack[undoIndex]) == true)
    {
        world->editor_clear_hitblobs();

        // always reload script so that error message updates
        action->load_wren_from_string();

        scrub_to_frame(currentFrame);

        ++undoIndex;
        undoStack.erase(undoStack.begin() + undoIndex, undoStack.end());
        undoStack.push_back(action->clone());

        modified = action->has_changes(*savedData);
    }
}

//============================================================================//

void ActionContext::do_undo_redo(bool redo)
{
    const size_t oldIndex = undoIndex;

    if (!redo && undoIndex > 0u) --undoIndex;
    if (redo && undoIndex < undoStack.size() - 1u) ++undoIndex;

    if (undoIndex != oldIndex)
    {
        world->editor_clear_hitblobs();

        action->apply_changes(*undoStack[undoIndex]);
        action->load_wren_from_string();

        scrub_to_frame(currentFrame);

        modified = action->has_changes(*savedData);
    }
}

//============================================================================//

void ActionContext::save_changes()
{
    JsonValue json;

    auto& blobs = json["blobs"] = JsonValue::object();
    auto& effects = json["effects"] = JsonValue::object();
    auto& emitters = json["emitters"] = JsonValue::object();

    for (const auto& [key, blob] : action->mBlobs)
        blob.to_json(blobs[key.c_str()]);

    for (const auto& [key, effect] : action->mEffects)
        effect.to_json(effects[key.c_str()]);

    for (const auto& [key, emitter] : action->mEmitters)
        emitter.to_json(emitters[key.c_str()]);

    sq::write_text_to_file (
        "assets/fighters/{}/actions/{}.json"_format(fighter->name, action->name),
        json.dump(2), true
    );
    sq::write_text_to_file (
        "assets/fighters/{}/actions/{}.wren"_format(fighter->name, action->name),
        action->mWrenSource, true
    );

    savedData = action->clone();
    modified = false;
}

//============================================================================//

void ActionContext::show_menu_items()
{
    if (ImGui::MenuItem("Reload HurtBlobs"))
    {
        fighter->initialise_hurtblobs();
        scrub_to_frame(currentFrame);
    }

    if (ImGui::MenuItem("Reload Sounds"))
    {
        fighter->initialise_sounds();
        scrub_to_frame(currentFrame);
    }

    if (ImGui::MenuItem("Reload Animations"))
    {
        fighter->initialise_animations();
        reset_timeline_length();
        scrub_to_frame(currentFrame);
    }
}

//============================================================================//

void ActionContext::show_widgets()
{
    show_widget_hitblobs();
    show_widget_effects();
    show_widget_emitters();
    show_widget_scripts();
    show_widget_timeline();
    editor.helper_show_widget_debug(&world->get_stage(), fighter);
}

//============================================================================//

void ActionContext::show_widget_hitblobs()
{
    if (editor.mDoResetDockHitblobs) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockHitblobs = false;

    const ImPlus::ScopeWindow window = { "HitBlobs", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.hash();

    //--------------------------------------------------------//

    const auto funcInit = [&](HitBlob& blob)
    {
        blob.action = action;
        // todo: should also be done for copy and rename
        for (const char* c = blob.get_key().c_str(); *c != '\0'; ++c)
            if (*c >= '0' && *c <= '9')
                { blob.index = uint8_t(*c - '0'); break; }
    };

    const auto funcEdit = [&](HitBlob& blob)
    {
        const ImPlus::ScopeItemWidth width = -120.f;

        ImPlus::InputValue("Index", blob.index, 1, "%u");

        ImPlus::Combo("Bone", fighter->get_armature().get_bone_names(), blob.bone, "(None)");

        editor.helper_edit_origin("Origin", *fighter, blob.bone, blob.origin);

        ImPlus::SliderValue("Radius", blob.radius, 0.05f, 2.f, "%.2f metres");

        ImPlus::InputValue("Damage", blob.damage, 1.f, "%.2f %");
        ImPlus::InputValue("FreezeMult", blob.freezeMult, 0.1f, "%.2f ×");
        ImPlus::InputValue("FreezeDiMult", blob.freezeDiMult, 0.1f, "%.2f ×");

        ImPlus::InputValue("KnockAngle", blob.knockAngle, 1.f, "%.2f degrees");
        ImPlus::InputValue("KnockBase", blob.knockBase, 1.f, "%.2f units");
        ImPlus::InputValue("KnockScale", blob.knockScale, 1.f, "%.2f units");

        ImPlus::ComboEnum("AngleMode", blob.angleMode);
        ImPlus::ComboEnum("FacingMode", blob.facingMode);
        ImPlus::ComboEnum("ClangMode", blob.clangMode);
        ImPlus::ComboEnum("Flavour", blob.flavour);

        ImPlus::Checkbox("IgnoreDamage", &blob.ignoreDamage); ImGui::SameLine(160.f);
        ImPlus::Checkbox("IgnoreWeight", &blob.ignoreWeight);

        ImPlus::Checkbox("CanHitGround", &blob.canHitGround); ImGui::SameLine(160.f);
        ImPlus::Checkbox("CanHitAir", &blob.canHitAir);

        // todo: make these combo boxes, or at least show a warning if the key doesn't exist
        ImGui::InputText("Handler", blob.handler.data(), blob.handler.buffer_size());
        ImGui::InputText("Sound", blob.sound.data(), blob.sound.buffer_size());
    };

    editor.helper_edit_objects(action->mBlobs, funcInit, funcEdit, nullptr);
}

//============================================================================//

void ActionContext::show_widget_effects()
{
    if (editor.mDoResetDockEffects) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockEffects = false;

    const ImPlus::ScopeWindow window = { "Effects", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.hash();

    //--------------------------------------------------------//

    const auto funcInit = [&](VisualEffect& effect)
    {
        effect.cache = &world->caches.effects;
        effect.fighter = fighter;
        effect.path = "fighters/{}/effects/{}"_format(fighter->name, effect.get_key());
        effect.handle = effect.cache->try_acquire(effect.path.c_str(), true);
    };

    const auto funcEdit = [&](VisualEffect& effect)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString("Path", effect.path))
            effect.handle = effect.cache->try_acquire(effect.path.c_str(), true);

        if (effect.handle == nullptr) ImPlus::LabelText("Resolved", "COULD NOT LOAD RESOURCE");
        else ImPlus::LabelText("Resolved", "assets/{}/..."_format(effect.path));

        ImPlus::Combo("Bone", fighter->get_armature().get_bone_names(), effect.bone, "(None)");

        bool changed = false;
        changed |= ImPlus::InputVector("Origin", effect.origin, 0, "%.4f");
        changed |= ImPlus::InputQuaternion("Rotation", effect.rotation, 0, "%.4f");
        changed |= ImPlus::InputVector("Scale", effect.scale, 0, "%.4f");

        if (changed == true)
            effect.localMatrix = maths::transform(effect.origin, effect.rotation, effect.scale);

        ImPlus::Checkbox("Anchored", &effect.anchored);
    };

    editor.helper_edit_objects(action->mEffects, funcInit, funcEdit, nullptr);
}

//============================================================================//

void ActionContext::show_widget_emitters()
{
    if (editor.mDoResetDockEmitters) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockEmitters = false;

    const ImPlus::ScopeWindow window = { "Emitters", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.hash();

    //--------------------------------------------------------//

    const auto funcInit = [&](Emitter& emitter)
    {
        emitter.fighter = fighter;
        emitter.colour.emplace_back(1.f, 1.f, 1.f);
    };

    const auto funcEdit = [&](Emitter& emitter)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        ImPlus::Combo("Bone", fighter->get_armature().get_bone_names(), emitter.bone, "(None)");

        ImPlus::SliderValue("Count", emitter.count, 0u, 120u, "%u");

        editor.helper_edit_origin("Origin", *fighter, emitter.bone, emitter.origin);

        ImPlus::InputVector("Velocity", emitter.velocity, 0, "%.4f");

        ImPlus::SliderValue("BaseOpacity", emitter.baseOpacity, 0.2f, 1.f, "%.3f");
        ImPlus::SliderValue("EndOpacity", emitter.endOpacity, 0.f, 5.f, "%.3f");
        ImPlus::SliderValue("EndScale", emitter.endScale, 0.f, 5.f, "%.3f");

        ImPlus::DragValueRange2("Lifetime", emitter.lifetime.min, emitter.lifetime.max, 1.f/8, 4u, 240u, "%d");
        ImPlus::DragValueRange2("BaseRadius", emitter.baseRadius.min, emitter.baseRadius.max, 1.f/320, 0.05f, 1.f, "%.3f");

        ImPlus::DragValueRange2("BallOffset", emitter.ballOffset.min, emitter.ballOffset.max, 1.f/80, 0.f, 2.f, "%.3f");
        ImPlus::DragValueRange2("BallSpeed", emitter.ballSpeed.min, emitter.ballSpeed.max, 1.f/80, 0.f, 10.f, "%.3f");

        ImPlus::DragValueRange2("DiscIncline", emitter.discIncline.min, emitter.discIncline.max, 1.f/320, -0.25f, 0.25f, "%.3f");
        ImPlus::DragValueRange2("DiscOffset", emitter.discOffset.min, emitter.discOffset.max, 1.f/80, 0.f, 2.f, "%.3f");
        ImPlus::DragValueRange2("DiscSpeed", emitter.discSpeed.min, emitter.discSpeed.max, 1.f/80, 0.f, 10.f, "%.3f");

        Vec3F* entryToDelete = nullptr;

        for (Vec3F* iter = emitter.colour.begin(); iter != emitter.colour.end(); ++iter)
        {
            const ImPlus::ScopeID idScope = iter;
            if (ImGui::Button("X")) entryToDelete = iter;
            ImGui::SameLine();
            ImPlus::InputColour("RGB (Linear)", *iter, ImGuiColorEditFlags_Float);
        }

        if (entryToDelete != nullptr)
        {
            emitter.colour.erase(entryToDelete);
            if (emitter.colour.empty())
                emitter.colour.emplace_back(1.f, 1.f, 1.f);
        }

        if (ImGui::Button("New Colour Entry") && !emitter.colour.full())
            emitter.colour.emplace_back(1.f, 1.f, 1.f);

        ImGui::InputText("Sprite", emitter.sprite.data(), emitter.sprite.capacity());
    };

    editor.helper_edit_objects(action->mEmitters, funcInit, funcEdit, nullptr);
}

//============================================================================//

void ActionContext::show_widget_scripts()
{
    if (editor.mDoResetDockScript) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockScript = false;

    const ImPlus::ScopeWindow window = { "Script", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.hash();

    //--------------------------------------------------------//

    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

    const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    const ImVec2 inputSize = { contentRegion.x, contentRegion.y - 160.f };

    ImPlus::InputStringMultiline("##Script", action->mWrenSource, inputSize, ImGuiInputTextFlags_NoUndoRedo);

    if (fighter->editorErrorMessage.empty() == false)
        ImPlus::TextWrapped(fighter->editorErrorMessage);
}

//============================================================================//

void ActionContext::show_widget_timeline()
{
    if (editor.mDoResetDockTimeline) ImGui::SetNextWindowDockID(editor.mDockDownId);
    editor.mDoResetDockTimeline = false;

    const ImPlus::ScopeWindow window = { "Timeline", ImGuiWindowFlags_AlwaysHorizontalScrollbar };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.hash();

    //--------------------------------------------------------//

    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

    const ImPlus::Style_ButtonTextAlign buttonAlign = {0.f, 0.f};
    const ImPlus::Style_SelectableTextAlign selectableAlign = {0.5f, 0.f};

    // todo: can this be done better using imgui's tables?

    for (int i = -1; i < timelineLength; ++i)
    {
        ImGui::SameLine();

        const String label = "{}"_format(i);

        const bool active = currentFrame == i;
        const bool open = ImPlus::IsPopupOpen(label);

        if (ImPlus::Selectable(label, active || open, 0, {20u, 20u}))
            ImPlus::OpenPopup(label);

        if (currentFrame != i)
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImPlus::MOUSE_RIGHT))
               scrub_to_frame(i);

        const ImVec2 rectMin = ImGui::GetItemRectMin();
        const ImVec2 rectMax = ImGui::GetItemRectMax();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Border));

        if (i == currentFrame)
        {
            const float blend = editor.mPreviewMode == PreviewMode::Pause ?
                                editor.mBlendValue : float(editor.mAccumulation / editor.mTickTime);
            const float middle = maths::mix(rectMin.x, rectMax.x, blend);

            const ImVec2 scrubberTop = { middle, rectMax.y + 2.f };
            const ImVec2 scrubberLeft = { middle - 3.f, scrubberTop.y + 8.f };
            const ImVec2 scrubberRight = { middle + 3.f, scrubberTop.y + 8.f };

            drawList->AddTriangleFilled(scrubberTop, scrubberLeft, scrubberRight, ImGui::GetColorU32(ImGuiCol_Border));
        }
    }
}

//============================================================================//

void ActionContext::reset_timeline_length()
{
    // todo: move to json or wren so we can properly handle extra actions

    const auto anim_length = [this](const SmallString& name)
    {
        const auto iter = fighter->mAnimations.find(name);
        if (iter == fighter->mAnimations.end())
        {
            sq::log_warning("could not find animation '{}'", name);
            return 80u;
        }
        return iter->second.anim.frameCount;
    };

    const SmallString& name = ctxKey.name;

    if      (name == "HopBack")    timelineLength = anim_length("JumpBack");
    else if (name == "HopForward") timelineLength = anim_length("JumpForward");
    else                           timelineLength = anim_length(name);

    currentFrame = std::min(currentFrame, timelineLength);
    timelineLength += 1; // extra time before looping
}

//============================================================================//

void ActionContext::setup_state_for_action()
{
    // todo: move to json or wren so we can properly handle extra actions

    Fighter::Attributes& attrs = fighter->attributes;
    Fighter::Variables& vars = fighter->variables;

    const SmallString& name = ctxKey.name;

    if (name == "Brake" || name == "BrakeTurn")
    {
        //fighter.change_state(fighter.mStates.at("Dash"));
        fighter->change_state(fighter->mStates.at("Neutral"));
        fighter->play_animation(fighter->mAnimations.at("DashLoop"), 0u, true);
        vars.velocity.x = attrs.dashSpeed + attrs.traction;
    }

    else if (name == "HopBack" || name == "HopForward")
    {
        fighter->change_state(fighter->mStates.at("JumpSquat"));
        fighter->play_animation(fighter->mAnimations.at("JumpSquat"), 0u, true);
        fighter->mAnimTimeDiscrete = fighter->mAnimation->anim.frameCount - 1u;
        vars.velocity.y = std::sqrt(2.f * attrs.hopHeight * attrs.gravity) + attrs.gravity * 0.5f;
    }

    else if (name == "JumpBack" || name == "JumpForward")
    {
        fighter->change_state(fighter->mStates.at("JumpSquat"));
        fighter->play_animation(fighter->mAnimations.at("JumpSquat"), 0u, true);
        fighter->mAnimTimeDiscrete = fighter->mAnimation->anim.frameCount - 1u;
        vars.velocity.y = std::sqrt(2.f * attrs.jumpHeight * attrs.gravity) + attrs.gravity * 0.5f;
    }

    else if (name.starts_with("Air") || name.starts_with("SpecialAir"))
    {
        fighter->change_state(fighter->mStates.at("Fall"));
        fighter->play_animation(fighter->mAnimations.at("FallLoop"), 0u, true);
        attrs.gravity = 0.f;
        vars.position = Vec2F(0.f, 1.f);
    }

    else
    {
        fighter->change_state(fighter->mStates.at("Neutral"));
        fighter->play_animation(fighter->mAnimations.at("NeutralLoop"), 0u, true);
    }
}

//============================================================================//

void ActionContext::scrub_to_frame(int frame)
{
    // wait for all in progress rendering to finish
    sq::VulkanContext::get().device.waitIdle();

    // clear and reset things
    //mController->wren_clear_history();
    world->set_rng_seed(editor.mRandomSeed);
    world->get_particle_system().clear();
    world->get_effect_system().clear();
    fighter->reset_everything();

    // actions have different starting state
    setup_state_for_action();

    // tick once to apply starting state and animation
    world->tick();

    // don't t-pose when blending the starting frame
    fighter->previous = fighter->current;
    fighter->debugPreviousPoseInfo = fighter->debugCurrentPoseInfo;

    // will activate the action when currentFrame >= 0
    fighter->editorStartAction = action;

    // finally, scrub to the desired frame
    editor.mSmashApp.get_audio_context().set_groups_ignored(sq::SoundGroup::Sfx, true);
    for (currentFrame = -1; currentFrame < frame; ++currentFrame)
        world->tick();
    editor.mSmashApp.get_audio_context().set_groups_ignored(sq::SoundGroup::Sfx, false);

    // this would be an assertion, but we want to avoid crashing the editor
    #ifdef SQEE_DEBUG
    if (fighter->activeAction == action && currentFrame >= 0 && fighter->editorErrorCause == nullptr)
    {
        const int actionFrame = int(fighter->activeAction->mCurrentFrame) - 1;
        if (currentFrame != actionFrame)
            sq::log_warning("out of sync: timeline = {}, action = {}", currentFrame, actionFrame);
    }
    #endif
}

//============================================================================//

void ActionContext::advance_frame(bool previous)
{
    if (previous == false)
    {
        if (++currentFrame >= timelineLength)
        {
            if (editor.mIncrementSeed) ++editor.mRandomSeed;
            scrub_to_frame(-1);
        }
        else world->tick();
    }
    else
    {
        if (currentFrame <= -1) scrub_to_frame(timelineLength - 1);
        else scrub_to_frame(currentFrame - 1);
    }
}
