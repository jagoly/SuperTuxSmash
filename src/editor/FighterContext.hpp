#pragma once

#include "setup.hpp"

#include "editor/BaseContext.hpp"

namespace sts {

//============================================================================//

struct EditorScene::FighterContext : BaseContext
{
    FighterContext(EditorScene& editor, String ctxKey);

    ~FighterContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    void show_widget_hurtblobs();

    //--------------------------------------------------------//

    FighterDef* fighterDef;

    struct UndoEntry
    {
        explicit UndoEntry(const FighterDef& def);

        std::map<SmallString, SoundEffect> sounds;
        std::map<TinyString, HurtBlobDef> hurtBlobs;

        bool has_changes(const FighterDef& def) const;
        void revert_changes(FighterDef& def) const;
    };

    std::unique_ptr<UndoEntry> savedData;
    std::vector<std::unique_ptr<UndoEntry>> undoStack;
};

//============================================================================//

} // namespace sts
