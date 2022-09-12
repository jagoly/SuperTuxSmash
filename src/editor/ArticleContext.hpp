#pragma once

#include "setup.hpp"

#include "editor/BaseContext.hpp"

namespace sts {

//============================================================================//

struct EditorScene::ArticleContext : BaseContext
{
    ArticleContext(EditorScene& editor, String ctxKey);

    ~ArticleContext() override;

    //--------------------------------------------------------//

    void apply_working_changes() override;

    void do_undo_redo(bool redo) override;

    void save_changes() override;

    void show_menu_items() override;

    void show_widgets() override;

    //--------------------------------------------------------//

    ArticleDef* articleDef;

    struct UndoEntry
    {
        explicit UndoEntry(const ArticleDef& def);

        std::map<SmallString, SoundEffect> sounds;
        std::map<TinyString, HitBlobDef> blobs;
        std::map<TinyString, VisualEffectDef> effects;
        std::map<TinyString, Emitter> emitters;
        String wrenSource;

        bool has_changes(const ArticleDef& def) const;
        void revert_changes(ArticleDef& def) const;
    };

    std::unique_ptr<UndoEntry> savedData;
    std::vector<std::unique_ptr<UndoEntry>> undoStack;
};

//============================================================================//

} // namespace sts
