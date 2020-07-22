#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiSystem.hpp>
#include <sqee/app/Scene.hpp>

#include "render/Renderer.hpp"

#include "game/Actions.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include <set>

namespace sts {

//============================================================================//

class EditorScene final : public sq::Scene
{
public: //====================================================//

    EditorScene(SmashApp& smashApp);

    ~EditorScene() override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event) override;

    void refresh_options() override;

    void show_imgui_widgets() override;

    enum class PreviewMode { Pause, Normal, Slow, Slower };

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    struct ActionKey
    {
        FighterEnum fighter; ActionType action;
        constexpr int hash() const { return int(fighter) * 256 + int(action); }
        constexpr bool operator==(const ActionKey& other) const { return hash() == other.hash(); }
        constexpr bool operator<(const ActionKey& other) const { return hash() < other.hash(); }
    };

    struct BaseContext
    {
        UniquePtr<FightWorld> world;
        UniquePtr<Renderer> renderer;

        Fighter* fighter;
        RenderObject* renderFighter;
    };

    struct ActionContext : public BaseContext
    {
        ActionKey key;

        UniquePtr<Action> savedData;
        bool modified = false;

        Vector<UniquePtr<Action>> undoStack;
        size_t undoIndex = 0u;

        std::map<TinyString, Vector<String>> buildErrors;

        int timelineLength = 0;
        int currentFrame = 0;
    };

    struct HurtblobsContext : public BaseContext
    {
        FighterEnum key;

        UniquePtr<decltype(Fighter::mHurtBlobs)> savedData;
        bool modified = false;

        Vector<UniquePtr<decltype(Fighter::mHurtBlobs)>> undoStack;
        size_t undoIndex = 0u;

        std::set<TinyString> hiddenKeys;
    };

    std::map<ActionKey, ActionContext> mActionContexts;
    std::map<FighterEnum, HurtblobsContext> mHurtblobsContexts;

    // todo: these should probably be a variant, or I should use dynamic cast
    BaseContext* mActiveContext = nullptr;
    ActionContext* mActiveActionContext = nullptr;
    HurtblobsContext* mActiveHurtblobsContext = nullptr;

    //--------------------------------------------------------//

    void impl_setup_docks();

    void impl_show_widget_toolbar();
    void impl_show_widget_navigator();

    void impl_show_widget_hitblobs();
    void impl_show_widget_emitters();
    void impl_show_widget_script();
    void impl_show_widget_hurtblobs();
    void impl_show_widget_timeline();

    void impl_show_widget_fighter();

    //--------------------------------------------------------//

    void create_base_context(FighterEnum fighterKey, BaseContext& ctx);

    ActionContext& get_action_context(ActionKey key);

    HurtblobsContext& get_hurtblobs_context(FighterEnum key);

    //--------------------------------------------------------//

    void apply_working_changes(ActionContext& ctx);
    void apply_working_changes(HurtblobsContext& ctx);

    void do_undo_redo(ActionContext& ctx, bool redo);
    void do_undo_redo(HurtblobsContext& ctx, bool redo);

    void save_changes(ActionContext& ctx);
    void save_changes(HurtblobsContext& ctx);

    //--------------------------------------------------------//

    void scrub_to_frame(ActionContext& ctx, int frame);
    void scrub_to_frame_current(ActionContext& ctx);

    void tick_action_context(ActionContext& ctx);

    //--------------------------------------------------------//

    ActionContext* mConfirmCloseActionCtx = nullptr;
    HurtblobsContext* mConfirmCloseHurtblobsCtx = nullptr;

    PreviewMode mPreviewMode = PreviewMode::Pause;

    bool mIncrementSeed = false;
    uint_fast32_t mRandomSeed = 0u;

    float mBlendValue = 1.f;

    //--------------------------------------------------------//

    ImGuiID mDockMainId = 0u;

    ImGuiID mDockNotRightId = 0u, mDockRightId = 0u;
    ImGuiID mDockNotDownId = 0u, mDockDownId = 0u;
    ImGuiID mDockNotLeftId = 0u, mDockLeftId = 0u;

    bool mWantResetDocks = true;

    bool mDoResetDockNavigator = false;
    bool mDoResetDockHitblobs = false;
    bool mDoResetDockEmitters = false;
    bool mDoResetDockScript = false;
    bool mDoResetDockHurtblobs = false;
    bool mDoResetDockTimeline = false;
    bool mDoResetDockFighter = false;

    bool mDoRestartAction = false;

    //--------------------------------------------------------//

    // instead of using static variables
    Vec3F mTempVec3F;
};

} // namespace sts

SQEE_ENUM_HELPER(sts::EditorScene::PreviewMode, Pause, Normal, Slow, Slower)
