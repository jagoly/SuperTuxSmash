#pragma once

#include "setup.hpp"

#include "editor/EditorScene.hpp"

namespace sts {

//============================================================================//

struct EditorScene::ActionContext : EditorScene::BaseContext
{
    ActionContext(EditorScene& editor, ActionKey key);

    ~ActionContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    void show_widget_hitblobs();

    void show_widget_effects();

    void show_widget_emitters();

    void show_widget_sounds();

    void show_widget_scripts();

    void show_widget_timeline();

    //--------------------------------------------------------//

    void reset_timeline_length();

    void setup_state_for_action();

    void scrub_to_frame(int frame);

    //void scrub_to_frame_current();

    void advance_frame(bool previous);

    //--------------------------------------------------------//

    const ActionKey ctxKey;

    Fighter* fighter;
    FighterAction* action;

    std::unique_ptr<FighterAction> savedData;
    std::vector<std::unique_ptr<FighterAction>> undoStack;

    int timelineLength = 0;
    int currentFrame = -1;
};

//============================================================================//

} // namespace sts
