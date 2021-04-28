#ifdef SQEE_WINDOWS
#include <sqee/redist/backward.hpp>
namespace backward { SignalHandling sh; }
#endif

//============================================================================//

#include "main/SmashApp.hpp"

int main(int argc, char** argv)
{
    sts::SmashApp app;

    return app.run(argc, argv);
}
