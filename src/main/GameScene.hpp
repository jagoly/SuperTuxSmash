#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiSystem.hpp>
#include <sqee/app/Scene.hpp>

#include "render/Renderer.hpp"

#include "game/FightWorld.hpp"
#include "game/Stage.hpp"
#include "game/Controller.hpp"

#include "main/Options.hpp"

//============================================================================//

namespace sts {

class GameScene final : public sq::Scene
{
public: //====================================================//

    GameScene(const sq::InputDevices& inputDevices, const Options& options);

    ~GameScene() override;

    //--------------------------------------------------------//

    void refresh_options();

    void handle_event(sq::Event event);

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    unique_ptr<FightWorld> mFightWorld;

    unique_ptr<Renderer> mRenderer;

    std::array<unique_ptr<Controller>, 4> mControllers;

    //--------------------------------------------------------//

    const sq::InputDevices& mInputDevices;

    const Options& mOptions;

    //--------------------------------------------------------//

    sq::GuiWidget mGeneralWidget;
    sq::GuiWidget mFightersWidget;

    void impl_show_general_window();
    void impl_show_fighters_window();
};

} // namespace sts
