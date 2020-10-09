#include "editor/EditorScene.hpp" // IWYU pragma: associated
#include "editor/Editor_Helpers.hpp"

#include "game/Action.hpp"
#include "game/Fighter.hpp"
#include "game/FightWorld.hpp"
#include "game/SoundEffect.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_sounds()
{
    if (mActiveActionContext == nullptr) return;

    if (mDoResetDockSounds) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockSounds = false;

    const ImPlus::ScopeWindow window = { "Sounds", 0 };
    if (window.show == false) return;

    //--------------------------------------------------------//

    ActionContext& ctx = *mActiveActionContext;
    Fighter& fighter = *ctx.fighter;
    Action& action = *fighter.get_action(ctx.key.action);

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    //--------------------------------------------------------//

    const auto funcInit = [&](SoundEffect& sound)
    {
        sound.cache = &ctx.world->sounds;
        sound.path = fmt::format("fighters/{}/sounds/{}", fighter.type, sound.get_key());
        if (sq::check_file_exists(sq::build_string("assets/", sound.path.c_str(), ".wav")))
            sound.handle = sound.cache->acquire(sound.path.c_str());
        sound.volume = 100.f; // very loud, in case you forget to set it
    };

    const auto funcEdit = [&](SoundEffect& sound)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString(" Resource", sound.path))
        {
            sound.handle = nullptr;
            if (sq::check_file_exists(sq::build_string("assets/", sound.path, ".wav")))
                sound.handle = sound.cache->acquire(sound.path.c_str());
        }

        ImPlus::Text(sq::build_string("  assets/", sound.path, ".wav"));
        if (sound.handle == nullptr) ImPlus::Text("FILE NOT FOUND");

        ImPlus::SliderValue(" Volume", sound.volume, 0.2f, 1.f, "%.2f ×");
    };

    //--------------------------------------------------------//

    helper_edit_objects(action.mSounds, funcInit, funcEdit);
}
