#pragma once

#include <sqee/app/Application.hpp>

namespace sts {

//============================================================================//

class SmashApp final : public sq::Application
{
public:

    //========================================================//

    SmashApp();

    void eval_test_init();

    void update_options();
    bool handle(sf::Event event);
};

//============================================================================//

} // namespace sts
