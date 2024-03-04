#include "editor/ActionContext.hpp"

#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/SoundEffect.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;
using ActionContext = EditorScene::ActionContext;

//============================================================================//

ActionContext::ActionContext(EditorScene& _editor, String _ctxKey)
    : BaseContext(_editor, std::move(_ctxKey))
{
    const auto fighterKey = StringView(ctxKey).substr(9, ctxKey.find('/', 9) - 9);
    const auto actionKey = StringView(ctxKey).substr(ctxKey.rfind('/') + 1);

    world->create_stage("TestZone");

    fighter = &world->create_fighter(fighterKey);
    fighter->controller = editor.mController.get();

    action = &fighter->mActions.at(actionKey);
    actionDef = const_cast<FighterActionDef*>(&action->def);

    savedData = std::make_unique<UndoEntry>(*actionDef);
    undoStack.push_back(std::make_unique<UndoEntry>(*actionDef));

    enumerate_source_files(fmt::format("assets/{}.wren", ctxKey), actionDef->wrenSource, savedData->wrenSource);

    // todo: move this to json, allow toggling opponent for other actions
    if (actionKey.starts_with("Grab") || actionKey.starts_with("Grabbed") || actionKey.starts_with("Throw"))
    {
        // todo: allow changing test opponent
        opponent = &world->create_fighter(fighterKey);
        // todo: give opponent its own controller
        opponent->controller = editor.mController.get();
    }

    reset_timeline_length();
    scrub_to_frame(-1, false);
}

ActionContext::~ActionContext() = default;

//============================================================================//

void ActionContext::apply_working_changes()
{
    if (undoStack[undoIndex]->has_changes(*actionDef) == true)
    {
        reset_objects();

        actionDef->interpret_module();
        action->initialise_script();

        scrub_to_frame(currentFrame, false);

        undoStack.erase(undoStack.begin() + (++undoIndex), undoStack.end());
        undoStack.push_back(std::make_unique<UndoEntry>(*actionDef));

        modified = savedData->has_changes(*actionDef);
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
        reset_objects();

        undoStack[undoIndex]->revert_changes(*actionDef);

        actionDef->interpret_module();
        action->initialise_script();

        scrub_to_frame(currentFrame, false);

        modified = savedData->has_changes(*actionDef);
    }
}

//============================================================================//

void ActionContext::save_changes()
{
    if (savedData->blobs != actionDef->blobs || savedData->effects != actionDef->effects || savedData->emitters != actionDef->emitters)
    {
        auto document = JsonMutDocument();
        auto json = document.assign(JsonMutObject(document));

        auto jBlobs = json.append("blobs", JsonMutObject(document));
        auto jEffects = json.append("effects", JsonMutObject(document));
        auto jEmitters = json.append("emitters", JsonMutObject(document));

        for (const auto& [key, blob] : actionDef->blobs)
            blob.to_json(jBlobs.append(key, JsonMutObject(document)), fighter->def.armature);

        for (const auto& [key, effect] : actionDef->effects)
            effect.to_json(jEffects.append(key, JsonMutObject(document)), fighter->def.armature);

        for (const auto& [key, emitter] : actionDef->emitters)
            emitter.to_json(jEmitters.append(key, JsonMutObject(document)), fighter->def.armature);

        sq::write_text_to_file(fmt::format("assets/{}.json", ctxKey), json.dump(true), true);
        sq::write_text_to_file(fmt::format("assets/{}.wren", ctxKey), actionDef->wrenSource, true);
    }

    savedData = std::make_unique<UndoEntry>(*actionDef);
    modified = false;
}

//============================================================================//

void ActionContext::show_menu_items()
{
    FighterDef& fighterDef = const_cast<FighterDef&>(fighter->def);

    if (ImGui::MenuItem("Reload HurtBlobs"))
    {
        fighterDef.hurtBlobs.clear();
        fighterDef.initialise_hurtblobs();
        fighter->mHurtBlobs.clear();
        fighter->initialise_hurtblobs();
        deferScrubToFrame = currentFrame;
    }

    if (ImGui::MenuItem("Reload Sounds"))
    {
        // todo: make sure there are no dangling pointers around
        fighterDef.sounds.clear();
        fighterDef.initialise_sounds(fmt::format("assets/{}/Sounds.json", fighterDef.directory));
        deferScrubToFrame = currentFrame;
    }

    if (ImGui::MenuItem("Reload Animations"))
    {
        // todo: make sure there are no dangling pointers around
        fighterDef.animations.clear();
        fighterDef.initialise_animations("assets/fighters/Animations.json");
        fighterDef.initialise_animations(fmt::format("assets/{}/Animations.json", fighterDef.directory));
        reset_timeline_length();
        deferScrubToFrame = std::min(currentFrame, timelineEnd);
    }
}

//============================================================================//

void ActionContext::show_widgets()
{
    show_widget_hitblobs(fighter->def.armature, actionDef->blobs);
    show_widget_effects(fighter->def.armature, actionDef->effects);
    show_widget_emitters(fighter->def.armature, actionDef->emitters);
    show_widget_scripts();
    show_widget_timeline();
    show_widget_debug();
}

//============================================================================//

void ActionContext::reset_timeline_length()
{
    // todo: move to json or wren so we can properly handle extra actions

    const auto anim_length = [this](const SmallString& name)
    {
        const auto iter = fighter->def.animations.find(name);
        if (iter == fighter->def.animations.end())
        {
            sq::log_warning("could not find animation '{}'", name);
            return 80u;
        }
        return iter->second.anim.frameCount;
    };

    // by default, assume anim name is the same as the action
    const SmallString& name = action->def.name;

    if      (name == "HopBack")      timelineEnd = anim_length("JumpBack");
    else if (name == "HopForward")   timelineEnd = anim_length("JumpForward");
    else if (name == "GrabStart")    timelineEnd = anim_length("GrabLoop");
    else if (name == "GrabbedStart") timelineEnd = anim_length("GrabbedStartLow");
    else if (name == "GrabbedFree")  timelineEnd = anim_length("GrabbedFreeLow");
    else                             timelineEnd = anim_length(name);

    timelineEnd += 1; // extra time before looping
}

//============================================================================//

ActionContext::UndoEntry::UndoEntry(const FighterActionDef& def)
    : blobs(def.blobs)
    , effects(def.effects)
    , emitters(def.emitters)
    , wrenSource(def.wrenSource) {}

bool ActionContext::UndoEntry::has_changes(const FighterActionDef& def) const
{
    return def.blobs != blobs ||
           def.effects != effects ||
           def.emitters != emitters ||
           def.wrenSource != wrenSource;
}

void ActionContext::UndoEntry::revert_changes(FighterActionDef& def) const
{
    def.blobs = blobs;
    def.effects = effects;
    def.emitters = emitters;
    def.wrenSource = wrenSource;
}
