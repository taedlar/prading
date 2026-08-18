#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <argparse/argparse.hpp>
#include <mudmux/mudmux.h>
#include <mudmux/comm.h>

static void process_command_line(int argc, char* argv[]);

int main (int argc, char* argv[]) {
    process_command_line(argc, argv);
    int exit_code = mudmux_run(nullptr);
    mudmux_deinit();
    return exit_code;
}

void process_command_line(int argc, char* argv[]) {
    int log_level = spdlog::level::warn; // keep quieter defaults for non-Debug builds
    argparse::ArgumentParser program (argv[0], "1.0");
    program.add_argument("-f", "--config").metavar("FILE").default_value(std::string("mud.conf"))
        .help("specify configuration file");
    program.add_argument("-i", "--input").metavar("FILE").default_value(std::string("stdin"))
        .help("specify input file (default: stdin)");
    program.add_argument("-o", "--output").metavar("FILE").default_value(std::string("stdout"))
        .help("specify output file (default: stdout)");
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
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl << program;
        std::exit(EXIT_FAILURE);
    }

    // call mudmux_init() to process configuration file (or use defaults if no file is specified)
    if (program.is_used("--config")) {
        std::string config_file = program.get<std::string>("--config");
        std::ifstream input(config_file);
        if (!input) {
            SPDLOG_ERROR ("failed to open configuration file: {}", config_file);
            std::exit(EXIT_FAILURE);
        }

        std::stringstream buffer;
        buffer << input.rdbuf();
        std::string config_yaml = buffer.str();

        if (!mudmux_init(config_yaml.c_str())) {
            std::exit(EXIT_FAILURE);
        }
    }
    else {
        SPDLOG_INFO ("no configuration file specified, using defaults");
        if (!mudmux_init(nullptr)) {
            std::exit(EXIT_FAILURE);
        }
    }

    if (program.is_used("--input") || program.is_used("--output")) {
        // standard input/output overrides console mode (no re-connection after EOF)
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
            mudmux_enable_standard_input(true);
    }
}
