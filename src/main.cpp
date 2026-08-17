#include "config.h"
#include <argparse/argparse.hpp>
#include <mudmux/mudmux.h>

static void process_command_line(int argc, char* argv[]);

int main (int argc, char* argv[]) {
    int exit_code = EXIT_SUCCESS;
    process_command_line(argc, argv);
    if (mudmux_init("") == 0) {
        mudmux_deinit();
    }
    SPDLOG_INFO("exiting ({})", exit_code);
    return exit_code;
}

void process_command_line(int argc, char* argv[]) {
    argparse::ArgumentParser program (argv[0], "1.0");
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl << program;
        std::exit(EXIT_FAILURE);
    }
}
