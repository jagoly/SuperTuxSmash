#pragma once

#include "setup.hpp"

#include "game/ActionEnums.hpp"
#include "main/MainEnums.hpp"

#include <sqee/app/Scene.hpp>
#include <sqee/objects/Texture.hpp>
#include <sqee/vk/Wrappers.hpp>

// IWYU pragma: no_include "game/Action.hpp"
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

    // only public so we can use SQEE_ENUM_HELPER
    enum class PreviewMode { Pause, Normal, Slow, Slower };

private: //===================================================//

    void update() override;

    void integrate(double elapsed, float blend) override;

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    struct ShrunkCubeMap;

    struct ActionKey
    {
        FighterEnum fighter; ActionType action;
        int hash() const { return int(fighter) * 256 + int(action); }
        bool operator==(const ActionKey& other) const { return hash() == other.hash(); }
        bool operator<(const ActionKey& other) const { return hash() < other.hash(); }
    };

    struct CubeMapView
    {
        void initialise(EditorScene& editor, uint level, vk::Image image, vk::Sampler sampler);
        std::array<vk::ImageView, 6u> imageViews;
        std::array<vk::DescriptorSet, 6u> descriptorSets;
    };

    //--------------------------------------------------------//

    struct BaseContext
    {
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<FightWorld> world;

        bool modified = false;
        size_t undoIndex = 0u;
    };

    struct ActionContext : public BaseContext
    {
        ActionKey key; Fighter* fighter;

        std::unique_ptr<Action> savedData;
        std::vector<std::unique_ptr<Action>> undoStack;

        int timelineLength = 0, currentFrame = -1;
    };

    struct HurtblobsContext : public BaseContext
    {
        FighterEnum key; Fighter* fighter;

        std::unique_ptr<std::pmr::map<TinyString, HurtBlob>> savedData;
        std::vector<std::unique_ptr<std::pmr::map<TinyString, HurtBlob>>> undoStack;
    };

    struct StageContext : public BaseContext
    {
        StageEnum key; Stage* stage;

        CubeMapView skybox;
        CubeMapView irradiance;
        std::array<CubeMapView, RADIANCE_LEVELS> radiance;

        bool irradianceModified = false;
        bool radianceModified = false;

        ~StageContext();
    };

    //--------------------------------------------------------//

    std::map<ActionKey, ActionContext> mActionContexts;
    std::map<FighterEnum, HurtblobsContext> mHurtblobsContexts;
    std::map<StageEnum, StageContext> mStageContexts;

    BaseContext* mActiveContext = nullptr;

    ActionContext* mActiveActionContext = nullptr;
    HurtblobsContext* mActiveHurtblobsContext = nullptr;
    StageContext* mActiveStageContext = nullptr;

    //--------------------------------------------------------//

    void impl_show_widget_toolbar();
    void impl_show_widget_navigator();

    void impl_show_widget_hitblobs();
    void impl_show_widget_effects();
    void impl_show_widget_emitters();
    void impl_show_widget_sounds();
    void impl_show_widget_script();
    void impl_show_widget_timeline();
    void impl_show_widget_hurtblobs();
    void impl_show_widget_stage();
    void impl_show_widget_cubemaps();

    void impl_show_widget_fighter();

    //--------------------------------------------------------//

    static uint get_default_timeline_length(const ActionContext& ctx);

    //--------------------------------------------------------//

    void initialise_base_context(BaseContext& ctx);

    void activate_action_context(ActionKey key);

    HurtblobsContext& get_hurtblobs_context(FighterEnum key);

    StageContext& get_stage_context(StageEnum key);

    //--------------------------------------------------------//

    void apply_working_changes(ActionContext& ctx);
    void apply_working_changes(HurtblobsContext& ctx);
    void apply_working_changes(StageContext& ctx);

    void do_undo_redo(ActionContext& ctx, bool redo);
    void do_undo_redo(HurtblobsContext& ctx, bool redo);
    void do_undo_redo(StageContext& ctx, bool redo);

    void save_changes(ActionContext& ctx);
    void save_changes(HurtblobsContext& ctx);
    void save_changes(StageContext& ctx);

    //--------------------------------------------------------//

    void scrub_to_frame(ActionContext& ctx, int frame);
    void scrub_to_frame_current(ActionContext& ctx);

    void tick_action_context(ActionContext& ctx);
    void scrub_to_frame_previous(ActionContext& ctx);

    //--------------------------------------------------------//

    ShrunkCubeMap shrink_cube_map_skybox(vk::ImageLayout layout, uint outputSize) const;

    void generate_cube_map_irradiance();
    void generate_cube_map_radiance();

    void update_cube_map_texture(sq::ImageStuff source, uint size, uint levels, sq::Texture& texture);

    //--------------------------------------------------------//

    // layouts for basic pipelines with a single image sampler
    vk::DescriptorSetLayout mImageProcessSetLayout;
    vk::PipelineLayout mImageProcessPipelineLayout;

    //--------------------------------------------------------//

    ActionContext* mConfirmCloseActionCtx = nullptr;
    HurtblobsContext* mConfirmCloseHurtblobsCtx = nullptr;
    StageContext* mConfirmCloseStageCtx = nullptr;
    uint mConfirmQuitNumUnsaved = 0u;

    PreviewMode mPreviewMode = PreviewMode::Pause;

    bool mIncrementSeed = false;
    uint_fast32_t mRandomSeed = 0u;

    float mBlendValue = 1.f;

    //--------------------------------------------------------//

    ImGuiID mDockMainId = 0u;

    ImGuiID mDockNotRightId = 0u, mDockRightId = 0u;
    ImGuiID mDockNotDownId = 0u, mDockDownId = 0u;
    ImGuiID mDockNotLeftId = 0u, mDockLeftId = 0u;

    bool mWantResetDocks = true;

    bool mDoResetDockNavigator = false;
    bool mDoResetDockHitblobs = false;
    bool mDoResetDockEffects = false;
    bool mDoResetDockEmitters = false;
    bool mDoResetDockSounds = false;
    bool mDoResetDockScript = false;
    bool mDoResetDockTimeline = false;
    bool mDoResetDockHurtblobs = false;
    bool mDoResetDockStage = false;
    bool mDoResetDockCubemaps = false;
    bool mDoResetDockFighter = false;

    bool mDoRestartAction = false;

    //--------------------------------------------------------//

    // make sure any source file that uses these includes Editor_Helpers.hpp

    template <class Map, class FuncInit, class FuncEdit>
    void helper_edit_objects(Map& objects, FuncInit funcInit, FuncEdit funcEdit);
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::EditorScene::PreviewMode, Pause, Normal, Slow, Slower)
