#pragma once

#include "setup.hpp"

#include "editor/BaseContext.hpp"

namespace sts {

//============================================================================//

struct EditorScene::ActionContext : BaseContext
{
    ActionContext(EditorScene& editor, String ctxKey);

    ~ActionContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    void reset_timeline_length();

    //--------------------------------------------------------//

    FighterActionDef* actionDef;

    struct UndoEntry
    {
        explicit UndoEntry(const FighterActionDef& def);

        std::map<TinyString, HitBlobDef> blobs;
        std::map<TinyString, VisualEffectDef> effects;
        std::map<TinyString, Emitter> emitters;
        String wrenSource;

        bool has_changes(const FighterActionDef& def) const;
        void revert_changes(FighterActionDef& def) const;
    };

    std::unique_ptr<UndoEntry> savedData;
    std::vector<std::unique_ptr<UndoEntry>> undoStack;
};

//============================================================================//

} // namespace sts
