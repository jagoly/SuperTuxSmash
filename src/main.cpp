#include <iostream>

#include <sqee/redist/lmccop.hpp>

#include <main/SmashApp.hpp>

enum Index { UNKNOWN, HELP };

const op::Descriptor usage[] =
{
    {UNKNOWN, 0, "", "",     op::Arg::None, "USAGE: sts-game [options]\n\nOptions:"},
    {HELP,    0, "", "help", op::Arg::None, " --help\n Show this message"},
    {0,0,0,0,0,0}
};

int main(int argc, char* argv[])
{
    argc-=(argc>0); argv+=(argc>0);

    op::Stats stats(usage, argc, argv);
    vector<op::Option> options(stats.options_max);
    vector<op::Option> buffer(stats.buffer_max);
    op::Parser parse(usage, argc, argv, options.data(), buffer.data());

    if (options[HELP] || options[UNKNOWN] || parse.error())
    {
        op::printUsage(std::cout, usage);
        return 1;
    }

    sts::SmashApp app;
    app.eval_test_init();
    return app.run();
}
