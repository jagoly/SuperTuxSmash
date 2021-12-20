#include "editor/EditorScene.hpp" // IWYU pragma: associated
#include "editor/Editor_Helpers.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/SoundEffect.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_sounds()
{
    if (mDoResetDockSounds) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockSounds = false;

    const ImPlus::ScopeWindow window = { "Sounds", 0 };
    if (window.show == false) return;

    ActionContext& ctx = *mActiveActionContext;
    Fighter& fighter = *ctx.fighter;
    FighterAction& action = *ctx.action;

    //--------------------------------------------------------//

    const auto funcInit = [&](SoundEffect& sound)
    {
        sound.cache = &ctx.world->caches.sounds;
        sound.path = "fighters/{}/sounds/{}"_format(fighter.name, sound.get_key());
        sound.handle = sound.cache->try_acquire(sound.path.c_str(), true);
        sound.volume = 100.f; // very loud, in case you forget to set it
    };

    const auto funcEdit = [&](SoundEffect& sound)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString(" Resource", sound.path))
            sound.handle = sound.cache->try_acquire(sound.path.c_str(), true);

        ImPlus::Text(sq::build_string("  assets/", sound.path, ".wav"));
        if (sound.handle == nullptr) ImPlus::Text("COULD NOT LOAD RESOURCE");

        ImPlus::SliderValue(" Volume", sound.volume, 0.2f, 1.f, "%.2f ×");
    };

    //--------------------------------------------------------//

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    helper_edit_objects(action.mSounds, funcInit, funcEdit);
}
