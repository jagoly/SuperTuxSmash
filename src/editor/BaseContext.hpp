#pragma once

#include "setup.hpp"

#include "editor/EditorScene.hpp"

namespace sq { class Armature; }

namespace sts {

//============================================================================//

struct EditorScene::BaseContext
{
    BaseContext(EditorScene& editor, String ctxKey);

    virtual ~BaseContext();

    //--------------------------------------------------------//

    struct SourceFile
    {
        String path;            // relative to executable
        String* sourcePtr;      // editable files only
        String sourceStr;       // non-editable files only
        String* savedSourcePtr; // editable files only
    };

    //--------------------------------------------------------//

    virtual void apply_working_changes() = 0;

    virtual void do_undo_redo(bool redo) = 0;

    virtual void save_changes() = 0;

    virtual void show_menu_items() = 0;

    virtual void show_widgets() = 0;

    //--------------------------------------------------------//

    void show_widget_hitblobs(const sq::Armature& armature, std::map<TinyString, HitBlobDef>& blobs);

    void show_widget_effects(const sq::Armature& armature, std::map<TinyString, VisualEffectDef>& effects);

    void show_widget_emitters(const sq::Armature& armature, std::map<TinyString, Emitter>& emitters);

    void show_widget_sounds(std::map<SmallString, SoundEffect>& sounds);

    void show_widget_scripts();

    void show_widget_timeline();

    void show_widget_debug();

    //--------------------------------------------------------//

    template <class Key, class Object, class FuncInit, class FuncEdit, class FuncBefore>
    void helper_edit_objects (
        std::map<Key, Object>& objects, FuncInit funcInit, FuncEdit funcEdit, FuncBefore funcBefore
    );

    void helper_edit_origin(StringView label, const sq::Armature& armature, int8_t bone, Vec3F& origin);

    //--------------------------------------------------------//

    void enumerate_source_files(String path, String& ref, String& saved);

    void reset_objects();

    void setup_state_for_action();

    void scrub_to_frame(int frame, bool resetObjects);

    void advance_frame(bool previous);

    //--------------------------------------------------------//

    EditorScene& editor;
    Renderer& renderer;

    const String ctxKey;

    std::unique_ptr<EditorCamera> camera;
    std::unique_ptr<World> world;

    bool modified = false;
    size_t undoIndex = 0u;

    int timelineBegin = 0;
    int timelineEnd = 0;
    int currentFrame = -1;

    std::optional<int> deferScrubToFrame;

    Stage* stage = nullptr;
    Fighter* fighter = nullptr;
    FighterAction* action = nullptr;

    Fighter* opponent = nullptr;

    std::vector<SourceFile> sourceFiles;
    int sourceFileIndex = -1;
};

//============================================================================//

} // namespace sts
