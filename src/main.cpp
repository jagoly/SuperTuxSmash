#include "main/SmashApp.hpp"

//============================================================================//

//#ifdef SQEE_MSVC
//#include <sqee/redist/backward.hpp>
//namespace backward { SignalHandling sh; }
//#endif

//============================================================================//

int main(int argc, char** argv)
{
    sts::SmashApp app;

    return app.run(argc, argv);
}
