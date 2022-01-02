#pragma once

#include "setup.hpp"

#include "editor/EditorScene.hpp"

namespace sts {

//============================================================================//

struct EditorScene::HurtBlobsContext : EditorScene::BaseContext
{
    HurtBlobsContext(EditorScene& editor, FighterEnum key);

    ~HurtBlobsContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    void show_widget_hurtblobs();

    //--------------------------------------------------------//

    const FighterEnum ctxKey;

    Fighter* fighter;

    std::unique_ptr<std::map<TinyString, HurtBlob>> savedData;
    std::vector<std::unique_ptr<std::map<TinyString, HurtBlob>>> undoStack;
};

//============================================================================//

} // namespace sts
