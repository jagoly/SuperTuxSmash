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

//============================================================================//

namespace sts {

class ActionEditor final : public sq::Scene
{
public: //====================================================//

    ActionEditor(SmashApp& smashApp);

    ~ActionEditor() override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event) override;

    void refresh_options() override;
    using ProcedurePair = std::pair<const String, Action::Procedure>;

    enum class PreviewMode { Pause, Normal, Slow, Slower };

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    // destroy these objects last
    UniquePtr<FightWorld> mFightWorld;
    UniquePtr<Renderer> mRenderer;

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    sq::GuiWidget widget_main_menu;
    sq::GuiWidget widget_hitblobs;
    sq::GuiWidget widget_emitters;
    sq::GuiWidget widget_procedures;
    sq::GuiWidget widget_timeline;

    void impl_show_widget_main_menu();
    void impl_show_widget_hitblobs();
    void impl_show_widget_emitters();
    void impl_show_widget_procedures();
    void impl_show_widget_timeline();

    sq::MessageReceiver <
        message::fighter_action_finished
    > receiver;

    void handle_message(const message::fighter_action_finished& msg);

    //--------------------------------------------------------//

    bool build_working_procedures();

    void apply_working_changes();

    void revert_working_changes();

    void update_sorted_procedures();

    void save_changed_actions();

    void scrub_to_frame(uint16_t frame);

    //--------------------------------------------------------//

    UniquePtr<Action> mWorkingAction;
    Action* mReferenceAction = nullptr;

    Vector<ProcedurePair*> mSortedProcedures;

    Vector<ActionType> mChangedActions;

    struct CtxSwitchAction
    {
        bool waitConfirm = false;
        ActionType actionType = ActionType::None;
    };

    Optional<CtxSwitchAction> ctxSwitchAction;

    //--------------------------------------------------------//

    Fighter* fighter = nullptr;
    PrivateFighter* privateFighter = nullptr;

    //--------------------------------------------------------//

    bool mRenderBlobsEnabled = true;
    bool mSortProceduresEnabled = true;
    bool mLiveEditEnabled = false;

    PreviewMode mPreviewMode = PreviewMode::Pause;

    uint64_t mRandomSeed = 1337ul;
};

} // namespace sts

SQEE_ENUM_HELPER(sts::ActionEditor::PreviewMode, Pause, Normal, Slow, Slower)
