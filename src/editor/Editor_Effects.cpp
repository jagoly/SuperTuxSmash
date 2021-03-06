#include "editor/EditorScene.hpp" // IWYU pragma: associated
#include "editor/Editor_Helpers.hpp"

#include "game/Action.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/VisualEffect.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_effects()
{
    if (mActiveActionContext == nullptr) return;

    if (mDoResetDockEffects) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockEffects = false;

    const ImPlus::ScopeWindow window = { "Effects", 0 };
    if (window.show == false) return;

    //--------------------------------------------------------//

    ActionContext& ctx = *mActiveActionContext;
    Fighter& fighter = *ctx.fighter;
    Action& action = *fighter.get_action(ctx.key.action);

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    //--------------------------------------------------------//

    const auto funcInit = [&](VisualEffect& effect)
    {
        effect.cache = &ctx.world->caches.effects;
        effect.fighter = ctx.fighter;

        effect.path = fmt::format("fighters/{}/effects/{}", fighter.type, effect.get_key());
        if (sq::check_file_exists(sq::build_string("assets/", effect.path.c_str(), "/Animation.json")))
            effect.handle = effect.cache->acquire(effect.path.c_str());

        effect.origin = { 0.f, 0.f, 0.f };
        effect.rotation = { 0.f, 0.f, 0.f, 1.f };
        effect.scale = { 1.f, 1.f, 1.f };
    };

    const auto funcEdit = [&](VisualEffect& effect)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString(" Resource", effect.path))
        {
            // todo: check for the directory here, handle missing files with exceptions
            effect.handle = nullptr;
            if (sq::check_file_exists(sq::build_string("assets/", effect.path, "/Animation.json")))
                effect.handle = effect.cache->acquire(effect.path.c_str());
        }

        ImPlus::Text(sq::build_string("  assets/", effect.path, "/..."));
        if (effect.handle == nullptr) ImPlus::Text("FILE(S) NOT FOUND");

        bool changed = false;
        changed |= ImPlus::InputVector(" Origin", effect.origin, 0, "%.4f");
        changed |= ImPlus::InputQuaternion(" Rotation", effect.rotation, 0, "%.4f");
        changed |= ImPlus::InputVector(" Scale", effect.scale, 0, "%.4f");

        if (changed == true)
            effect.localMatrix = maths::transform(effect.origin, effect.rotation, effect.scale);

        ImPlus::Checkbox("Anchored ", &effect.anchored);
    };

    //--------------------------------------------------------//

    helper_edit_objects(action.mEffects, funcInit, funcEdit);
}
