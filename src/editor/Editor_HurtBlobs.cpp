#include "editor/Editor_HurtBlobs.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/FighterState.hpp"
#include "game/HurtBlob.hpp"

#include "editor/Editor_Helpers.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;
using HurtBlobsContext = EditorScene::HurtBlobsContext;

//============================================================================//

HurtBlobsContext::HurtBlobsContext(EditorScene& _editor, FighterEnum _key)
    : BaseContext(_editor, StageEnum::TestZone), ctxKey(_key)
{
    ctxTypeString = "HurtBlobs";
    ctxKeyString = sq::enum_to_string(ctxKey);

    world->add_fighter(std::make_unique<Fighter>(*world, ctxKey, 0u));
    world->get_fighter(0u).controller = editor.mController.get();

    fighter = &world->get_fighter(0u);

    fighter->mAnimation = nullptr;
    fighter->variables.facing = 2;
    world->tick();

    savedData = std::make_unique<decltype(Fighter::mHurtBlobs)>(fighter->mHurtBlobs);
    undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(fighter->mHurtBlobs));
}

HurtBlobsContext::~HurtBlobsContext() = default;

//============================================================================//

void HurtBlobsContext::apply_working_changes()
{
    if (fighter->mHurtBlobs != *undoStack[undoIndex])
    {
        world->editor_clear_hurtblobs();

        for (auto& [key, blob] : fighter->mHurtBlobs)
            world->enable_hurtblob(&blob);

        world->tick();

        undoStack.erase(undoStack.begin() + ++undoIndex, undoStack.end());
        undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(fighter->mHurtBlobs));

        modified = fighter->mHurtBlobs != *savedData;
    }
}

//============================================================================//

void HurtBlobsContext::do_undo_redo(bool redo)
{
    const size_t oldIndex = undoIndex;

    if (!redo && undoIndex > 0u) --undoIndex;
    if (redo && undoIndex < undoStack.size() - 1u) ++undoIndex;

    if (undoIndex != oldIndex)
    {
        world->editor_clear_hurtblobs();

        fighter->mHurtBlobs = *undoStack[undoIndex];

        for (auto& [key, blob] : fighter->mHurtBlobs)
            world->enable_hurtblob(&blob);

        world->tick();

        modified = fighter->mHurtBlobs != *savedData;
    }
}

//============================================================================//

void HurtBlobsContext::save_changes()
{
    JsonValue json;

    for (const auto& [key, blob] : fighter->mHurtBlobs)
        blob.to_json(json[key.c_str()]);

    sq::write_text_to_file("assets/fighters/{}/HurtBlobs.json"_format(fighter->name), json.dump(2));

    *savedData = fighter->mHurtBlobs;
    modified = false;
}

//============================================================================//

void HurtBlobsContext::show_menu_items()
{
    // nothing here yet
}

void HurtBlobsContext::show_widgets()
{
    show_widget_hurtblobs();
    editor.helper_show_widget_debug(&world->get_stage(), fighter);
}

//============================================================================//

void HurtBlobsContext::show_widget_hurtblobs()
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

    editor.helper_edit_objects(fighter->mHurtBlobs, funcInit, funcEdit);
}
