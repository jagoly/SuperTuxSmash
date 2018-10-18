#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiSystem.hpp>
#include <sqee/app/Scene.hpp>

#include "render/Renderer.hpp"

#include "game/FightWorld.hpp"
#include "game/Stage.hpp"
#include "game/Controller.hpp"
#include "game/Actions.hpp"

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

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    UniquePtr<FightWorld> mFightWorld;

    UniquePtr<Renderer> mRenderer;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    sq::GuiWidget widget_list_actions;

    sq::GuiWidget widget_list_blobs;
    sq::GuiWidget widget_edit_blob;

    sq::GuiWidget widget_timeline;

    void impl_show_list_actions_widget();

    void impl_show_list_blobs_widget();
    void impl_show_edit_blob_widget();

    void impl_show_timeline_widget();

    //--------------------------------------------------------//

    Action::Type mActionType = Action::Type::None;

    bool mManualTickEnabled = true;

    //--------------------------------------------------------//

    Fighter* mFighter = nullptr;
    Action* mAction = nullptr;
};

} // namespace sts
