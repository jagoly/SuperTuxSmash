#include <SFML/Window/Event.hpp>

#include <sqee/scripts/ChaiEngine.hpp>
#include <sqee/app/DebugOverlay.hpp>
#include <sqee/debug/Logging.hpp>

#include <main/Options.hpp>

#include <scenes/MenuScene.hpp>
#include <scenes/GameScene.hpp>

#include "SmashApp.hpp"

using namespace sts;
namespace maths = sq::maths;

//============================================================================//

SmashApp::SmashApp()
{
    std::cout << "foo" << std::endl;
    mScenes.emplace_back(new GameScene(*this));
}

//============================================================================//

void SmashApp::eval_test_init()
{
//    try { mChaiEngine->eval_file("assets/test_init.chai"); }
//    catch (chai::exception::eval_error& err)
//    { sq::log_error(err.pretty_print()); }
}

//============================================================================//

void SmashApp::update_options()
{
    Options::get().Window_Size = OPTION_WindowSize;
    sq::Application::update_options();
}

//============================================================================//

bool SmashApp::handle(sf::Event event)
{
    if (sq::Application::handle(event)) return true;

    auto notify = [this](uint value, const string& message, vector<string> options)
    { this->mDebugOverlay->notify(message + options[value], 6u); };

    static Options& options = Options::get();

    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::V)
        {
            OPTION_VerticalSync = !OPTION_VerticalSync;
            notify(OPTION_VerticalSync, "vsync set to ", {"OFF", "ON"});
            update_options(); return true;
        }

        if (event.key.code == sf::Keyboard::B)
        {
            options.Bloom_Enable = !options.Bloom_Enable;
            notify(options.Bloom_Enable, "bloom set to ", {"OFF", "ON"});
            update_options(); return true;
        }

        if (event.key.code == sf::Keyboard::O)
        {
            options.SSAO_Quality = ++options.SSAO_Quality == 3 ? 0 : options.SSAO_Quality;
            notify(options.SSAO_Quality, "ssao set to ", {"OFF", "LOW", "HIGH"});
            update_options(); return true;
        }

        if (event.key.code == sf::Keyboard::A)
        {
            options.FSAA_Quality = ++options.FSAA_Quality == 3 ? 0 : options.FSAA_Quality;
            notify(options.FSAA_Quality, "fsaa set to ", {"OFF", "FXAA", "SMAA"});
            update_options(); return true;
        }
    }

    return false;
}
