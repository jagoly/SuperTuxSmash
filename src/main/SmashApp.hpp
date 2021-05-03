#pragma once

#include "setup.hpp"

#include <sqee/app/Application.hpp>

#include <sqee/vk/VulkWindow.hpp> // IWYU pragma: export
#include <sqee/vk/VulkInputDevices.hpp>  // IWYU pragma: export
#include <sqee/app/DebugOverlay.hpp> // IWYU pragma: export
#include <sqee/app/AudioContext.hpp> // IWYU pragma: export
#include <sqee/vk/VulkGuiSystem.hpp> // IWYU pragma: export
#include <sqee/app/Scene.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

class SmashApp final : public sq::Application
{
public: //====================================================//

    SmashApp();

    ~SmashApp() override;

    //--------------------------------------------------------//

    void start_game(GameSetup setup);

    void start_action_editor();

    void return_to_main_menu();

    //--------------------------------------------------------//

    sq::VulkWindow& get_window() { return *mWindow; }
    sq::VulkInputDevices& get_input_devices() { return *mInputDevices; }
    sq::DebugOverlay& get_debug_overlay() { return *mDebugOverlay; }
    sq::AudioContext& get_audio_context() { return *mAudioContext; }
    sq::VulkGuiSystem& get_gui_system() { return *mGuiSystem; }

    Options& get_options() { return *mOptions; }
    ResourceCaches& get_resource_caches() { return *mResourceCaches; }

private: //===================================================//

    void initialise(std::vector<String> args) override;

    void update(double elapsed) override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event);

    void refresh_options();

    void populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf);

    //--------------------------------------------------------//

    std::unique_ptr<sq::VulkWindow> mWindow;
    std::unique_ptr<sq::VulkInputDevices> mInputDevices;
    std::unique_ptr<sq::DebugOverlay> mDebugOverlay;
    std::unique_ptr<sq::AudioContext> mAudioContext;
    std::unique_ptr<sq::VulkGuiSystem> mGuiSystem;

    std::unique_ptr<Options> mOptions;
    std::unique_ptr<ResourceCaches> mResourceCaches;

    std::unique_ptr<sq::Scene> mActiveScene;
};

//============================================================================//

} // namespace sts
