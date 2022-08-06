#pragma once

#include "setup.hpp"

#include "editor/EditorScene.hpp"

namespace sts {

//============================================================================//

struct EditorScene::FighterContext : EditorScene::BaseContext
{
    FighterContext(EditorScene& editor, TinyString key);

    ~FighterContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    void show_widget_hurtblobs();
    void show_widget_sounds();

    //--------------------------------------------------------//

    const TinyString ctxKey;

    Fighter* fighter;
    FighterDef* fighterDef;

    struct UndoEntry
    {
        std::map<TinyString, HurtBlobDef> hurtBlobs;
        std::map<SmallString, SoundEffect> sounds;

        bool has_changes(const FighterDef& fighterDef) const;
    };

    std::unique_ptr<UndoEntry> savedData;
    std::vector<std::unique_ptr<UndoEntry>> undoStack;
};

//============================================================================//

} // namespace sts
