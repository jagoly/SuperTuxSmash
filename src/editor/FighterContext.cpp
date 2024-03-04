#include "editor/FighterContext.hpp"

#include "game/Fighter.hpp"
#include "game/HurtBlob.hpp"
#include "game/SoundEffect.hpp"
#include "game/World.hpp"

#include "editor/BaseContextImpl.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Armature.hpp>

using namespace sts;
using FighterContext = EditorScene::FighterContext;

//============================================================================//

FighterContext::FighterContext(EditorScene& _editor, String _ctxKey)
    : BaseContext(_editor, std::move(_ctxKey))
{
    const auto fighterKey = StringView(ctxKey).substr(9);

    world->create_stage("TestZone");

    fighter = &world->create_fighter(fighterKey);
    fighter->controller = editor.mController.get();
    fighterDef = const_cast<FighterDef*>(&fighter->def);

    savedData = std::make_unique<UndoEntry>(*fighterDef);
    undoStack.push_back(std::make_unique<UndoEntry>(*fighterDef));

    fighter->mAnimPlayer.animation = nullptr;
    fighter->variables.facing = 2;
    world->tick();
    world->tick();
}

FighterContext::~FighterContext() = default;

//============================================================================//

void FighterContext::apply_working_changes()
{
    if (undoStack[undoIndex]->has_changes(*fighterDef) == true)
    {
        fighter->mHurtBlobs.clear();
        fighter->initialise_hurtblobs();

        world->tick();

        undoStack.erase(undoStack.begin() + (++undoIndex), undoStack.end());
        undoStack.push_back(std::make_unique<UndoEntry>(*fighterDef));

        modified = savedData->has_changes(*fighterDef);
    }
}

//============================================================================//

void FighterContext::do_undo_redo(bool redo)
{
    const size_t oldIndex = undoIndex;

    if (!redo && undoIndex > 0u) --undoIndex;
    if (redo && undoIndex < undoStack.size() - 1u) ++undoIndex;

    if (undoIndex != oldIndex)
    {
        undoStack[undoIndex]->revert_changes(*fighterDef);

        fighter->mHurtBlobs.clear();
        fighter->initialise_hurtblobs();

        world->tick();

        modified = savedData->has_changes(*fighterDef);
    }
}

//============================================================================//

void FighterContext::save_changes()
{
    if (savedData->hurtBlobs != fighterDef->hurtBlobs)
    {
        auto document = JsonMutDocument();
        auto json = document.assign(JsonMutObject(document));

        for (const auto& [key, def] : fighterDef->hurtBlobs)
            def.to_json(json.append(key, JsonMutObject(document)), fighterDef->armature);

        sq::write_text_to_file(fmt::format("assets/{}/HurtBlobs.json", ctxKey), json.dump(true), true);
    }

    if (savedData->sounds != fighterDef->sounds)
    {
        JsonMutDocument document;
        auto json = document.assign(JsonMutObject(document));

        for (const auto& [key, def] : fighterDef->sounds)
            def.to_json(json.append(key, JsonMutObject(document)));

        sq::write_text_to_file(fmt::format("assets/{}/Sounds.json", ctxKey), json.dump(true), true);
    }

    savedData = std::make_unique<UndoEntry>(*fighterDef);
    modified = false;
}

//============================================================================//

void FighterContext::show_menu_items()
{
    // nothing here yet
}

//============================================================================//

void FighterContext::show_widgets()
{
    show_widget_hurtblobs();
    show_widget_sounds(fighterDef->sounds);
    show_widget_debug();
}

//============================================================================//

void FighterContext::show_widget_hurtblobs()
{
    if (editor.mDoResetDockHurtblobs) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockHurtblobs = false;

    const ImPlus::Scope_Window window = { "HurtBlobs", 0 };
    if (window.show == false) return;

    IMPLUS_WITH(Scope_ID) = ImStrv(ctxKey);

    //--------------------------------------------------------//

    const auto funcInit = [&](HurtBlobDef& /*def*/)
    {
        // nothing to do
    };

    const auto funcEdit = [&](HurtBlobDef& def)
    {
        IMPLUS_WITH(Scope_ItemWidth) = -120.f;

        ImPlus::ComboIndex("Bone", fighterDef->armature.get_bone_names(), def.bone, "(None)");

        ImPlus::ComboEnum("Region", def.region);

        helper_edit_origin("OriginA", fighterDef->armature, def.bone, def.originA);
        helper_edit_origin("OriginB", fighterDef->armature, def.bone, def.originB);

        ImPlus::SliderValue("Radius", def.radius, 0.05f, 1.5f, "%.2f metres");
    };

    helper_edit_objects(fighterDef->hurtBlobs, funcInit, funcEdit, nullptr);
}

//============================================================================//

FighterContext::UndoEntry::UndoEntry(const FighterDef& def)
    : sounds(def.sounds)
    , hurtBlobs(def.hurtBlobs) {}

bool FighterContext::UndoEntry::has_changes(const FighterDef& def) const
{
    return def.sounds != sounds ||
           def.hurtBlobs != hurtBlobs;
}

void FighterContext::UndoEntry::revert_changes(FighterDef& def) const
{
    def.sounds = sounds;
    def.hurtBlobs = hurtBlobs;
}
