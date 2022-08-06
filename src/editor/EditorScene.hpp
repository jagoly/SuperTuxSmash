#pragma once

#include "setup.hpp"

#include <sqee/app/Scene.hpp>
#include <sqee/objects/Texture.hpp>
#include <sqee/vk/Wrappers.hpp>

// IWYU pragma: no_include "game/FightWorld.hpp"
// IWYU pragma: no_include "render/Renderer.hpp"

namespace sts {

//============================================================================//

class EditorScene final : public sq::Scene
{
public: //====================================================//

    EditorScene(SmashApp& smashApp);

    ~EditorScene() override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event) override;

    void refresh_options_destroy() override;

    void refresh_options_create() override;

    void show_imgui_widgets() override;

    void populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf) override;

    //--------------------------------------------------------//

    struct FighterInfo
    {
        TinyString name;
        std::vector<SmallString> animations;
        std::vector<SmallString> actions;
        std::vector<SmallString> states;
    };

    struct ActionKey
    {
        TinyString fighter; SmallString action;
        bool operator==(const ActionKey& other) const { return std::memcmp(this, &other, sizeof(ActionKey)) == 0; }
        bool operator<(const ActionKey& other) const { return std::memcmp(this, &other, sizeof(ActionKey)) < 0; }
        StringView hash() const { return StringView(reinterpret_cast<const char*>(this), sizeof(ActionKey)); };
    };

    struct ShrunkCubeMap
    {
        sq::ImageStuff image;
        vk::DescriptorSet descriptorSet;
    };

    struct CubeMapView
    {
        void initialise(EditorScene& editor, uint level, vk::Image image, vk::Sampler sampler);
        std::array<vk::ImageView, 6u> imageViews;
        std::array<vk::DescriptorSet, 6u> descriptorSets;
    };

    struct BaseContext
    {
        BaseContext(EditorScene& editor, TinyString stage);

        virtual ~BaseContext();

        virtual void apply_working_changes() = 0;
        virtual void do_undo_redo(bool redo) = 0;
        virtual void save_changes() = 0;
        virtual void show_menu_items() {};
        virtual void show_widgets() = 0;

        EditorScene& editor;

        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<EditorCamera> camera;
        std::unique_ptr<World> world;

        bool modified = false;
        size_t undoIndex = 0u;

        String ctxTypeString, ctxKeyString;
    };

    struct ActionContext; friend ActionContext;
    struct FighterContext; friend FighterContext;
    struct StageContext; friend StageContext;

    enum class PreviewMode { Pause, Normal, Slow, Slower };

private: //===================================================//

    void update() override;

    void integrate(double elapsed, float blend) override;

    //--------------------------------------------------------//

    void impl_confirm_quit_unsaved(bool returnToMenu);

    void impl_show_widget_toolbar();
    void impl_show_widget_navigator();

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    std::unique_ptr<Controller> mController;

    FighterInfo mFighterInfoCommon;
    std::vector<FighterInfo> mFighterInfos;
    std::vector<TinyString> mStageNames;

    std::map<ActionKey, ActionContext> mActionContexts;
    std::map<TinyString, FighterContext> mFighterContexts;
    std::map<TinyString, StageContext> mStageContexts;

    BaseContext* mActiveContext = nullptr;

    //--------------------------------------------------------//

    // layouts for basic pipelines with a single image sampler
    vk::DescriptorSetLayout mImageProcessSetLayout;
    vk::PipelineLayout mImageProcessPipelineLayout;

    //--------------------------------------------------------//

    BaseContext* mConfirmCloseContext = nullptr;
    String mConfirmQuitUnsaved = "";
    bool mConfirmQuitReturnToMenu = false;

    PreviewMode mPreviewMode = PreviewMode::Pause;

    bool mIncrementSeed = false;
    uint_fast32_t mRandomSeed = 0u;

    float mBlendValue = 1.f;

    //--------------------------------------------------------//

    ImGuiID mDockMainId = 0u;

    ImGuiID mDockDownId = 0u, mDockNotDownId = 0u;
    ImGuiID mDockLeftId = 0u, mDockNotLeftId = 0u;
    ImGuiID mDockRightId = 0u, mDockNotRightId = 0u;

    bool mWantResetDocks = true;

    // todo: surely there's a better way to reset layout?
    bool mDoResetDockNavigator = false;
    bool mDoResetDockHitblobs = false;
    bool mDoResetDockEffects = false;
    bool mDoResetDockEmitters = false;
    bool mDoResetDockScript = false;
    bool mDoResetDockTimeline = false;
    bool mDoResetDockHurtblobs = false;
    bool mDoResetDockSounds = false;
    bool mDoResetDockStage = false;
    bool mDoResetDockCubemaps = false;
    bool mDoResetDockDebug = false;

    //--------------------------------------------------------//

    // make sure any source file that uses these includes Editor_Helpers.hpp

    template <class Key, class Object, class FuncInit, class FuncEdit, class FuncBefore>
    void helper_edit_objects (
        std::map<Key, Object>& objects, FuncInit funcInit, FuncEdit funcEdit, FuncBefore funcBefore
    );

    void helper_edit_origin(const char* label, Fighter& fighter, int8_t bone, Vec3F& origin);

    void helper_show_widget_debug(Stage* stage, Fighter* fighter);
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::EditorScene::PreviewMode, Pause, Normal, Slow, Slower)
