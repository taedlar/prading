#include "config.h"
#include "main.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <argparse/argparse.hpp>
#include <mudmux/mudmux.h>
#include <mudmux/comm.h>
#include <mudmux/hooks.h>

static void process_command_line(int argc, char* argv[]);
static int run_server();

#ifndef _WIN32
#include <signal.h>
static void signal_handler(int signal) {
    SPDLOG_INFO("received signal {}, shutting down...", signal);
    mudmux_shutdown(); // to exit the mudmux_run() event loop
}
#endif

int main (int argc, char* argv[]) {
    int exit_code = EXIT_FAILURE; // default to failure unless everything succeeds
#ifndef _WIN32
    // register signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif

    // parse command-line arguments before initializing mudmux (transport layer) settings or exiting the program
    process_command_line(argc, argv);

    // register transport-layer callbacks for mudmux
    mudmux_register_hook (HOOK_CONNECT, on_connect);
    mudmux_register_hook (HOOK_TRANSPORT_READY, on_transport_ready);
    mudmux_register_hook (HOOK_DISCONNECT, on_disconnect);

    exit_code = run_server();

    mudmux_deinit(); // deinitialize the transport layer and cleanup resources
    return exit_code;
}

int run_server() {
    Engine engine;
    World world(engine);

    // TODO: Initialize the world with zones, mobs, and other game entities here.

    // The transport layer borrows the world as its hook context. Destruction
    // occurs in reverse construction order: world first, then engine.
    return mudmux_run(&world);
}

void process_command_line(int argc, char* argv[]) {
    int log_level = spdlog::level::warn; // keep quieter defaults for non-Debug builds

    argparse::ArgumentParser program (PROGRAM_NAME, PROGRAM_VERSION);
    program.add_argument("-f", "--config").metavar("FILE").default_value(std::string("mud.conf"))
        .help("specify configuration file");
    program.add_argument("-i", "--input").metavar("FILE").default_value(std::string("stdin"))
        .help("specify local input file and connect as console user");
    program.add_argument("-o", "--output").metavar("FILE").default_value(std::string("stdout"))
        .help("specify local output file to receive output for console user");
    program.add_argument("-V", "--verbose").default_value(false).implicit_value(true).nargs(0)
        .action([&](const auto & /*unused*/) {
            if (log_level > spdlog::level::trace)
                log_level--;
        })
        .append() // -V: info, -VV: debug, -VVV: trace (debug/trace disabled in release builds)
        .help("increase verbosity of logging output");

    try {
        program.parse_args(argc, argv);
        spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level));
        SPDLOG_DEBUG ("log level set to {}", spdlog::level::to_string_view(spdlog::get_level()));

        // propagate log level to submodules
        mudmux_set_log_level(static_cast<spdlog::level::level_enum>(log_level));
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl << program;
        std::exit(EXIT_FAILURE);
    }

    if (program.is_used("--config")) {
        // The configuration file is shared by all modules. Each module can read its own
        // configuration items from the YAML file while ignoring the rest.
        std::string config_file = program.get<std::string>("--config");
        std::ifstream input(config_file);
        if (!input) {
            SPDLOG_ERROR ("failed to open configuration file: {}", config_file);
            std::exit(EXIT_FAILURE);
        }

        std::stringstream buffer;
        buffer << input.rdbuf();
        std::string config_yaml = buffer.str();

        if (!mudmux_init(config_yaml.c_str())) { // initialize transport-layer settings
            std::exit(EXIT_FAILURE);
        }
    }
    else {
        SPDLOG_INFO ("no configuration file specified, using defaults");
        if (!mudmux_init(nullptr)) { // initialize transport-layer settings with defaults
            std::exit(EXIT_FAILURE);
        }
    }

    if (program.is_used("--input") || program.is_used("--output")) {
        // using local --input/--output overrides console mode in configuration
        mudmux_enable_console(false);
        std::string input_file_str = program.get<std::string>("--input");
        std::string output_file_str = program.get<std::string>("--output");
        const char* input_file = (input_file_str != "stdin") ? input_file_str.c_str() : nullptr; // stdin by default
        const char* output_file = (output_file_str != "stdout") ? output_file_str.c_str() : nullptr; // stdout by default
        if (comm_abstract_add_file(input_file, output_file, COMM_SLOT_CONSOLE, 0) < 0) { // set up file/stdin and stdout
            SPDLOG_ERROR ("failed to open input/output files");
            std::exit(EXIT_FAILURE);
        }
        // Enable standard input handling only for stdin (not for file input)
        if (!input_file)
            mudmux_enable_standard_input(true); // unlike console mode, no re-connection after EOF
    }
}
