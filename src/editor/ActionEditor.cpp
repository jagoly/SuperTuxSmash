#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Logging.hpp>

#include "DebugGlobals.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"

#include "game/ActionBuilder.hpp"

#include "editor/ActionEditor.hpp"

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

ActionEditor::ActionEditor(SmashApp& smashApp)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    widget_list_actions.func = [this]() { impl_show_list_actions_widget(); };
    widget_list_blobs.func = [this]() { impl_show_list_blobs_widget(); };
    widget_edit_blob.func = [this]() { impl_show_edit_blob_widget(); };
    widget_timeline.func = [this]() { impl_show_timeline_widget(); };

    //--------------------------------------------------------//

    dbg.renderBlobs = true;

    mFightWorld = std::make_unique<FightWorld>(GameMode::Editor);
    mRenderer = std::make_unique<Renderer>(GameMode::Editor, mSmashApp.get_options());

    //--------------------------------------------------------//

    auto stage = std::make_unique<TestZone_Stage>(*mFightWorld);
    auto renderStage = std::make_unique<TestZone_Render>(*mRenderer, static_cast<TestZone_Stage&>(*stage));

    mFightWorld->set_stage(std::move(stage));
    mRenderer->add_object(std::move(renderStage));

    //--------------------------------------------------------//

    auto fighter = std::make_unique<Sara_Fighter>(0u, *mFightWorld);
    auto renderFighter = std::make_unique<Sara_Render>(*mRenderer, static_cast<Sara_Fighter&>(*fighter));

    //auto fighter = std::make_unique<Tux_Fighter>(0u, *mFightWorld);
    //auto renderFighter = std::make_unique<Tux_Render>(*mRenderer, static_cast<Tux_Fighter&>(*fighter));

    mFighter = fighter.get();

    mFightWorld->add_fighter(std::move(fighter));
    mRenderer->add_object(std::move(renderFighter));
}

ActionEditor::~ActionEditor() = default;

//============================================================================//

void ActionEditor::handle_event(sq::Event event)
{
    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::Space)
        {
            mFightWorld->tick();
        }
    }

    if (event.type == sq::Event::Type::Mouse_Scroll)
    {
        auto& camera = static_cast<EditorCamera&>(mRenderer->get_camera());
        camera.update_from_scroll(event.data.scroll.delta);
    }
}

//============================================================================//

void ActionEditor::refresh_options()
{
    mRenderer->refresh_options();
}

//============================================================================//

void ActionEditor::update()
{
    if (ImGui::GetIO().WantCaptureMouse == false)
    {
        const bool leftPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Left);
        const bool rightPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Right);
        const Vec2F position = Vec2F(mSmashApp.get_input_devices().get_cursor_location(true));

        auto& camera = static_cast<EditorCamera&>(mRenderer->get_camera());

        camera.update_from_mouse(leftPressed, rightPressed, position);
    }
}

//============================================================================//

void ActionEditor::render(double elapsed)
{
    const float accum = float(elapsed);
    const float blend = mManualTickEnabled ? 1.f : float(mAccumulation / mTickTime);

    mRenderer->render_objects(accum, blend);
    mRenderer->render_particles(mFightWorld->get_particle_system(), accum, blend);

    if (dbg.renderBlobs == true)
    {
        mRenderer->render_blobs(mFightWorld->get_hit_blobs());
        mRenderer->render_blobs(mFightWorld->get_hurt_blobs());
    }

    mRenderer->finish_rendering();
}

//============================================================================//

void ActionEditor::impl_show_list_actions_widget()
{
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowSizeConstraints({200, 0}, {200, ImGui::FromScreenBottom(20+150+20+20)});
    ImGui::SetNextWindowPos({20, 20});

    const auto prevActionType = mActionType;

    if (ImGui::Begin("choose_action", nullptr, flags))
    {
        if (mAction != nullptr)
        {
            const ImGui::ScopeFont font = ImGui::FONT_BOLD;
            ImGui::Text("Active: %s"_fmt_(enum_to_string(mAction->get_type())));
            ImGui::Separator();
        }

        for (int index = 0; index < int(Action::Type::Count); ++index)
        {
            const auto actionType = Action::Type(index);
            if (ImGui::Selectable(enum_to_string(actionType), actionType == mActionType))
            {
                mActionType = actionType;
            }
        }
    }

    if (mActionType != prevActionType)
    {
        mAction = mFighter->get_action(mActionType);
        std::cout << ActionBuilder::serialise_as_json(*mAction).dump(2) << std::endl;
    }

    ImGui::End();
}

//----------------------------------------------------------------------------//

void ActionEditor::impl_show_list_blobs_widget()
{

}

//----------------------------------------------------------------------------//

void ActionEditor::impl_show_edit_blob_widget()
{

}

//----------------------------------------------------------------------------//

void ActionEditor::impl_show_timeline_widget()
{
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowSize({ImGui::FromScreenRight(20+20), 150});
    ImGui::SetNextWindowPos({20, ImGui::FromScreenBottom(150+20)});

    if (ImGui::Begin("timeline", nullptr, flags))
    {
    }

    ImGui::End();
}
