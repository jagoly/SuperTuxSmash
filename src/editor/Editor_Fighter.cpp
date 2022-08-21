#include "editor/Editor_Fighter.hpp"

#include "game/Fighter.hpp"
#include "game/HurtBlob.hpp"
#include "game/SoundEffect.hpp"
#include "game/World.hpp"

#include "editor/Editor_Helpers.hpp"

#include <sqee/app/AudioContext.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;
using FighterContext = EditorScene::FighterContext;

//============================================================================//

FighterContext::FighterContext(EditorScene& _editor, TinyString _key)
    : BaseContext(_editor, "TestZone"), ctxKey(_key)
{
    ctxTypeString = "Fighter";
    ctxKeyString = _key;

    fighter = &world->create_fighter(ctxKey);
    fighter->controller = editor.mController.get();

    fighter->mAnimPlayer.animation = nullptr;
    fighter->variables.facing = 2;
    world->tick();
    world->tick();

    fighterDef = const_cast<FighterDef*>(&fighter->def);

    savedData = std::make_unique<UndoEntry>();
    *savedData = { fighterDef->hurtBlobs, fighterDef->sounds };

    undoStack.emplace_back(std::make_unique<UndoEntry>());
    *undoStack.back() = { fighterDef->hurtBlobs, fighterDef->sounds };
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

        undoStack.erase(undoStack.begin() + ++undoIndex, undoStack.end());

        undoStack.emplace_back(std::make_unique<UndoEntry>());
        *undoStack.back() = { fighterDef->hurtBlobs, fighterDef->sounds };

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
        fighterDef->hurtBlobs = undoStack[undoIndex]->hurtBlobs;
        fighterDef->sounds = undoStack[undoIndex]->sounds;

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
        JsonValue json;

        for (const auto& [key, def] : fighterDef->hurtBlobs)
            def.to_json(json[key.c_str()], fighterDef->armature);

        sq::write_text_to_file (
            fmt::format("assets/{}/HurtBlobs.json", fighterDef->directory),
            json.dump(2), true
        );
    }

    if (savedData->sounds != fighterDef->sounds)
    {
        JsonValue json;

        for (const auto& [key, sound] : fighterDef->sounds)
            sound.to_json(json[key.c_str()]);

        sq::write_text_to_file (
            fmt::format("assets/{}/Sounds.json", fighterDef->directory),
            json.dump(2), true
        );
    }

    *savedData = { fighterDef->hurtBlobs, fighterDef->sounds };
    modified = false;
}

//============================================================================//

void FighterContext::show_menu_items()
{
    // nothing here yet
}

void FighterContext::show_widgets()
{
    show_widget_hurtblobs();
    show_widget_sounds();
    editor.helper_show_widget_debug(&world->get_stage(), fighter);
}

//============================================================================//

void FighterContext::show_widget_hurtblobs()
{
    if (editor.mDoResetDockHurtblobs) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockHurtblobs = false;

    const ImPlus::ScopeWindow window = { "HurtBlobs", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyIdScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const auto funcInit = [&](HurtBlobDef& /*def*/)
    {
        // nothing to do
    };

    const auto funcEdit = [&](HurtBlobDef& def)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        ImPlus::ComboIndex("Bone", fighter->def.armature.get_bone_names(), def.bone, "(None)");

        ImPlus::ComboEnum("Region", def.region);

        editor.helper_edit_origin("OriginA", *fighter, def.bone, def.originA);
        editor.helper_edit_origin("OriginB", *fighter, def.bone, def.originB);

        ImPlus::SliderValue("Radius", def.radius, 0.05f, 1.5f, "%.2f metres");
    };

    editor.helper_edit_objects(fighterDef->hurtBlobs, funcInit, funcEdit, nullptr);
}

//============================================================================//

void FighterContext::show_widget_sounds()
{
    if (editor.mDoResetDockSounds) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockSounds = false;

    const ImPlus::ScopeWindow window = { "Sounds", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const auto funcInit = [&](SoundEffect& sound)
    {
        sound.cache = &world->caches.sounds;
        sound.path = fmt::format("{}/sounds/{}", fighterDef->directory, sound.get_key());
        sound.handle = sound.cache->try_acquire(sound.path.c_str(), true);
    };

    const auto funcEdit = [&](SoundEffect& sound)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString("Path", sound.path))
            sound.handle = sound.cache->try_acquire(sound.path.c_str(), true);

        if (sound.handle == nullptr) ImPlus::LabelText("Resolved", "COULD NOT LOAD RESOURCE");
        else ImPlus::LabelText("Resolved", fmt::format("assets/{}.wav", sound.path));

        ImPlus::SliderValue("Volume", sound.volume, 0.2f, 1.f, "%.2f ×");
    };

    const auto funcBefore = [&](SoundEffect sound)
    {
        ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x * 0.5f + 1.f);
        if (ImGui::Button("Play") && sound.handle != nullptr)
            world->audio.play_sound(sound.handle.get(), sq::SoundGroup::Sfx, sound.volume, false);
        ImGui::SameLine();
    };

    editor.helper_edit_objects(fighterDef->sounds, funcInit, funcEdit, funcBefore);
}

//============================================================================//

bool FighterContext::UndoEntry::has_changes(const FighterDef& fighterDef) const
{
    return hurtBlobs != fighterDef.hurtBlobs || sounds != fighterDef.sounds;
}
