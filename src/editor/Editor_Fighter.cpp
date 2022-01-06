#include "editor/Editor_Fighter.hpp"

#include "game/Fighter.hpp"
#include "game/FighterState.hpp"
#include "game/HurtBlob.hpp"
#include "game/SoundEffect.hpp"
#include "game/World.hpp"

#include "editor/Editor_Helpers.hpp"

#include <sqee/app/AudioContext.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;
using FighterContext = EditorScene::FighterContext;

//============================================================================//

FighterContext::FighterContext(EditorScene& _editor, FighterEnum _key)
    : BaseContext(_editor, StageEnum::TestZone), ctxKey(_key)
{
    ctxTypeString = "Fighter";
    ctxKeyString = sq::enum_to_string(ctxKey);

    world->add_fighter(std::make_unique<Fighter>(*world, ctxKey, 0u));
    world->get_fighter(0u).controller = editor.mController.get();

    fighter = &world->get_fighter(0u);

    fighter->mAnimation = nullptr;
    fighter->variables.facing = 2;
    world->tick();

    savedData = std::make_unique<UndoEntry>();
    *savedData = { fighter->mHurtBlobs, fighter->mSounds };

    undoStack.emplace_back(std::make_unique<UndoEntry>());
    *undoStack.back() = { fighter->mHurtBlobs, fighter->mSounds };
}

FighterContext::~FighterContext() = default;

//============================================================================//

void FighterContext::apply_working_changes()
{
    if (undoStack[undoIndex]->has_changes(*fighter) == true)
    {
        world->editor_clear_hurtblobs();

        for (auto& [key, blob] : fighter->mHurtBlobs)
            world->enable_hurtblob(&blob);

        world->tick();

        undoStack.erase(undoStack.begin() + ++undoIndex, undoStack.end());

        undoStack.emplace_back(std::make_unique<UndoEntry>());
        *undoStack.back() = { fighter->mHurtBlobs, fighter->mSounds };

        modified = savedData->has_changes(*fighter);
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
        world->editor_clear_hurtblobs();

        fighter->mHurtBlobs = undoStack[undoIndex]->hurtBlobs;
        fighter->mSounds = undoStack[undoIndex]->sounds;

        for (auto& [key, blob] : fighter->mHurtBlobs)
            world->enable_hurtblob(&blob);

        world->tick();

        modified = savedData->has_changes(*fighter);
    }
}

//============================================================================//

void FighterContext::save_changes()
{
    if (savedData->hurtBlobs != fighter->mHurtBlobs)
    {
        JsonValue json;

        for (const auto& [key, blob] : fighter->mHurtBlobs)
            blob.to_json(json[key.c_str()]);

        sq::write_text_to_file("assets/fighters/{}/HurtBlobs.json"_format(fighter->name), json.dump(2));
    }

    if (savedData->sounds != fighter->mSounds)
    {
        JsonValue json;

        for (const auto& [key, sound] : fighter->mSounds)
            sound.to_json(json[key.c_str()]);

        sq::write_text_to_file("assets/fighters/{}/Sounds.json"_format(fighter->name), json.dump(2));
    }

    *savedData = { fighter->mHurtBlobs, fighter->mSounds };
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

    const ImPlus::ScopeID ctxKeyIdScope = int(ctxKey);

    //--------------------------------------------------------//

    const auto funcInit = [&](HurtBlob& blob)
    {
        blob.fighter = fighter;
    };

    const auto funcEdit = [&](HurtBlob& blob)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        ImPlus::Combo("Bone", fighter->get_armature().get_bone_names(), blob.bone, "(None)");

        ImPlus::ComboEnum("Region", blob.region);

        editor.helper_edit_origin("OriginA", *fighter, blob.bone, blob.originA);
        editor.helper_edit_origin("OriginB", *fighter, blob.bone, blob.originB);

        ImPlus::SliderValue("Radius", blob.radius, 0.05f, 1.5f, "%.2f metres");
    };

    editor.helper_edit_objects(fighter->mHurtBlobs, funcInit, funcEdit, nullptr);
}

//============================================================================//

void FighterContext::show_widget_sounds()
{
    if (editor.mDoResetDockSounds) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockSounds = false;

    const ImPlus::ScopeWindow window = { "Sounds", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = int(ctxKey);

    //--------------------------------------------------------//

    const auto funcInit = [&](SoundEffect& sound)
    {
        sound.cache = &world->caches.sounds;
        sound.path = "fighters/{}/sounds/{}"_format(fighter->name, sound.get_key());
        sound.handle = sound.cache->try_acquire(sound.path.c_str(), true);
    };

    const auto funcEdit = [&](SoundEffect& sound)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString("Path", sound.path))
            sound.handle = sound.cache->try_acquire(sound.path.c_str(), true);

        if (sound.handle == nullptr) ImPlus::LabelText("Resolved", "COULD NOT LOAD RESOURCE");
        else ImPlus::LabelText("Resolved", "assets/{}.wav"_format(sound.path));

        ImPlus::SliderValue("Volume", sound.volume, 0.2f, 1.f, "%.2f ×");
    };

    const auto funcBefore = [&](SoundEffect sound)
    {
        ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x * 0.5f + 1.f);
        if (ImGui::Button("Play") && sound.handle != nullptr)
            world->audio.play_sound(sound.handle.get(), sq::SoundGroup::Sfx, sound.volume, false);
        ImGui::SameLine();
    };

    editor.helper_edit_objects(fighter->mSounds, funcInit, funcEdit, funcBefore);
}

//============================================================================//

bool FighterContext::UndoEntry::has_changes(const Fighter& fighter) const
{
    return hurtBlobs != fighter.mHurtBlobs || sounds != fighter.mSounds;
}
