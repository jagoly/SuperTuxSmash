#pragma once

#include "setup.hpp"

#include <sqee/app/Scene.hpp>
#include <sqee/vk/Vulkan.hpp>
#include <sqee/vk/Wrappers.hpp>

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
        std::vector<SmallString> articles;
        std::vector<SmallString> animations;
        std::vector<SmallString> actions;
        std::vector<SmallString> states;
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

    struct BaseContext; friend BaseContext;
    struct ActionContext; friend ActionContext;
    struct ArticleContext; friend ArticleContext;
    struct FighterContext; friend FighterContext;
    struct StageContext; friend StageContext;

    enum class PreviewMode { Pause, Normal, Slow, Slower };

private: //===================================================//

    void update() override;

    void integrate(double elapsed, float blend) override;

    //--------------------------------------------------------//

    bool context_has_timeline() const;

    void confirm_quit_unsaved(bool returnToMenu);

    void show_widget_toolbar();
    void show_widget_navigator();

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    std::unique_ptr<Renderer> mRenderer;
    std::unique_ptr<Controller> mController;

    FighterInfo mFighterInfoCommon;
    std::vector<FighterInfo> mFighterInfos;
    std::vector<String> mArticlePaths;
    std::vector<TinyString> mStageNames;

    std::map<String, ActionContext> mActionContexts;
    std::map<String, ArticleContext> mArticleContexts;
    std::map<String, FighterContext> mFighterContexts;
    std::map<String, StageContext> mStageContexts;

    BaseContext* mActiveContext = nullptr;

    //--------------------------------------------------------//

    // layouts for basic pipelines with a single image sampler
    vk::DescriptorSetLayout mImageProcessSetLayout;
    vk::PipelineLayout mImageProcessPipelineLayout;

    //--------------------------------------------------------//

    BaseContext* mConfirmCloseContext = nullptr;
    String mConfirmQuitUnsaved = "";
    bool mConfirmQuitReturnToMenu = false;

    int mActiveNavTab = 0;

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
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::EditorScene::PreviewMode, Pause, Normal, Slow, Slower)
