/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include "core.h"
#include "command_line.h"
#include <ply-build-repo/Workspace.h>
#include "commands.h"
#include <ply-build-repo/BuildFolder.h>
#include <ply-build-repo/Common.h>
#include <ply-build-repo/Repository.h>

using namespace ply::build;

//   ▄▄   ▄▄     ▄▄
//  ▄██▄▄ ▄▄  ▄▄▄██ ▄▄  ▄▄
//   ██   ██ ██  ██ ██  ██
//   ▀█▄▄ ██ ▀█▄▄██ ▀█▄▄██
//                   ▄▄▄█▀

void cmd_tidy(CommandLine& cl) {
    cl.check_for_unused_args();
    tidy_source();
}

//   ▄▄                                ▄▄
//  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄  ▄██▄▄
//   ██    ▄▄▄██ ██  ▀▀ ██  ██ ██▄▄██  ██
//   ▀█▄▄ ▀█▄▄██ ██     ▀█▄▄██ ▀█▄▄▄   ▀█▄▄
//                       ▄▄▄█▀

void cmd_target(CommandLine& cl) {
    StringView cmd = cl.next_arg();
    if (cmd.is_empty())
        goto print_usage;

    if (prefix_match(cmd, "add")) {
        StringView target_name = cl.next_arg();
        if (target_name.is_empty()) {
            Error.log("Expected target name after 'add'");
            goto print_usage;
        }

        cl.check_for_unused_args();
        return;
    } else {
        Error.log("Unrecognized target command '{}'", cmd);
    }

print_usage:;
}

//                                             ▄▄
//   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄
//  ██  ██ ██▄▄██ ██  ██ ██▄▄██ ██  ▀▀  ▄▄▄██  ██   ██▄▄██
//  ▀█▄▄██ ▀█▄▄▄  ██  ██ ▀█▄▄▄  ██     ▀█▄▄██  ▀█▄▄ ▀█▄▄▄
//   ▄▄▄█▀

void cmd_generate(CommandLine& cl) {
    cl.check_for_unused_args();

    Common::initialize();
    Repository::create();

    BuildFolder_t bf;
    String path =
        Path.join(Workspace.path, "data/build", Workspace.current_build_folder);
    bf.load(path);
    generate(&bf);
}

//                       ▄▄            ▄▄ ▄▄▄      ▄▄
//  ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ██▄▄▄  ▄▄  ▄▄ ▄▄  ██   ▄▄▄██
//  ██  ██ ██  ▀▀ ██▄▄██ ██  ██ ██  ██ ██  ██  ██  ██
//  ██▄▄█▀ ██     ▀█▄▄▄  ██▄▄█▀ ▀█▄▄██ ██ ▄██▄ ▀█▄▄██
//  ██

void cmd_prebuild(CommandLine& cl) {
    StringView build_folder_name = cl.next_arg();
    if (build_folder_name.is_empty()) {
        Error.log("Expected build folder name");
        exit(1);
    }
    StringView config_name = cl.next_arg();
    if (config_name.is_empty()) {
        Error.log("Expected configuration name");
        exit(1);
    }
    cl.check_for_unused_args();

    Common::initialize();
    Repository::create();

    BuildFolder_t bf;
    String path =
        Path.join(Workspace.path, "data/build", build_folder_name);
    bf.load(path);
    do_prebuild_steps(&bf, config_name);
}

//  ▄▄                    ▄▄           ▄▄
//  ██▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██  ██  ██   ▀█▄▄▄   ██   ██  ▀▀  ▄▄▄██ ██  ██
//  ██▄▄█▀ ▀█▄▄█▀ ▀█▄▄█▀  ▀█▄▄  ▄▄▄█▀  ▀█▄▄ ██     ▀█▄▄██ ██▄▄█▀
//                                                        ██

void cmd_bootstrap(CommandLine& cl) {
    StringView config_name;
    cl.find_option("config", &config_name);
    cl.check_for_unused_args();

    Common::initialize();
    Repository::create();

    BuildFolder_t bf;
    bf.abs_path = Path.join(Workspace.path, "data/build/crowbar");
    bf.solution_name = "crowbar";
    bf.root_targets.append("crowbar");

    write_bootstrap(&bf, 0);
}

//
//   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄█▀ ██▄▄█▀ ▀█▄▄▄  ██  ██
//         ██

void cmd_open(CommandLine& cl) {
    cl.check_for_unused_args();

    BuildFolder_t bf;
    String path =
        Path.join(Workspace.path, "data/build", Workspace.current_build_folder);
    if (!bf.load(path)) {
        exit(1);
    }

    command_open(&bf);
}

//                   ▄▄
//   ▄▄▄▄  ▄▄▄▄   ▄▄▄██  ▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ██    ██  ██ ██  ██ ██▄▄██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄▄ ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄▄  ▀█▄▄██ ▀█▄▄▄  ██  ██
//                              ▄▄▄█▀

void cmd_codegen(CommandLine& cl) {
    cl.check_for_unused_args();
    do_codegen();
}

//  ▄▄     ▄▄          ▄▄▄                ▄▄
//  ██▄▄▄  ▄▄  ▄▄▄▄▄  ██    ▄▄▄▄  ▄▄▄▄▄  ▄██▄▄
//  ██  ██ ██ ██  ██ ▀██▀▀ ██  ██ ██  ██  ██
//  ██▄▄█▀ ██ ▀█▄▄██  ██   ▀█▄▄█▀ ██  ██  ▀█▄▄
//             ▄▄▄█▀

void cmd_bigfont(CommandLine& cl) {
    MemOutStream out;
    if (StringView word = cl.next_arg()) {
        out << word;
    }
    while (StringView word = cl.next_arg()) {
        out << " " << word;
    }
    cl.check_for_unused_args();

    print_bigfont(out.move_to_string());
}

//                         ▄▄▄  ▄▄▄  ▄▄
//   ▄▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄   ██   ██  ██▄▄▄   ▄▄▄▄  ▄▄  ▄▄
//  ▀█▄▄▄  ██ ██ ██  ▄▄▄██  ██   ██  ██  ██ ██  ██  ▀██▀
//   ▄▄▄█▀ ██ ██ ██ ▀█▄▄██ ▄██▄ ▄██▄ ██▄▄█▀ ▀█▄▄█▀ ▄█▀▀█▄
//

void cmd_smallbox(CommandLine& cl) {
    MemOutStream out;
    if (StringView word = cl.next_arg()) {
        out << word;
    }
    while (StringView word = cl.next_arg()) {
        out << " " << word;
    }
    cl.check_for_unused_args();

    print_smallbox(out.move_to_string());
}

//    ▄▄▄        ▄▄▄      ▄▄
//   ██    ▄▄▄▄   ██   ▄▄▄██  ▄▄▄▄  ▄▄▄▄▄
//  ▀██▀  ██  ██  ██  ██  ██ ██▄▄██ ██  ▀▀
//   ██   ▀█▄▄█▀ ▄██▄ ▀█▄▄██ ▀█▄▄▄  ██
//

void cmd_folder(CommandLine& cl) {
    StringView cmd = cl.next_arg();
    if (cmd.is_empty())
        goto print_usage;

    if (prefix_match(cmd, "create")) {
        // create
        StringView name = cl.next_arg();
        if (!name) {
            Error.log("Expected folder name");
            exit(1);
        }
        cl.check_for_unused_args();
        if (create_build_folder(name)) {
            Console.out().format("Current build folder is now '{}'\n", name);
        }
        return;

    } else if (prefix_match(cmd, "list")) {
        // list
        cl.check_for_unused_args();
        Array<String> folder_names = get_build_folders();
        OutStream out = Console.out();
        for (StringView name : folder_names) {
            out << name;
            if (name == Workspace.current_build_folder) {
                out << " (current)";
            }
            out << '\n';
        }
        return;

    } else if (prefix_match(cmd, "set")) {
        // set
        StringView name = cl.next_arg();
        if (!name) {
            Error.log("Expected folder name");
            exit(1);
        }
        cl.check_for_unused_args();
        if (set_build_folder(name)) {
            Console.out().format("Current build folder is now '{}'\n", name);
        }
        return;

    } else {
        Error.log("Unrecognized command '{}'", cmd);
        exit(1);
    }

print_usage:;
}

//                  ▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄▄ ▄▄▄▄▄
//  ██ ██ ██  ▄▄▄██ ██ ██  ██
//  ██ ██ ██ ▀█▄▄██ ██ ██  ██
//

int main(int argc, char* argv[]) {
#if PLY_TARGET_WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    Workspace.load();

    CommandLine cl{argc, argv};
    StringView category = cl.next_arg();
    if (prefix_match(category, "tidy")) {
        cmd_tidy(cl);
    } else if (prefix_match(category, "target")) {
        cmd_target(cl);
    } else if (prefix_match(category, "generate")) {
        cmd_generate(cl);
    } else if (prefix_match(category, "bootstrap")) {
        cmd_bootstrap(cl);
    } else if (prefix_match(category, "open")) {
        cmd_open(cl);
    } else if (prefix_match(category, "codegen")) {
        cmd_codegen(cl);
    } else if (prefix_match(category, "bigfont")) {
        cmd_bigfont(cl);
    } else if (prefix_match(category, "smallbox")) {
        cmd_smallbox(cl);
    } else if (prefix_match(category, "folder")) {
        cmd_folder(cl);
    } else if (prefix_match(category, "prebuild")) {
        cmd_prebuild(cl);
    } else {
        Error.log("Unrecognized command \"{}\"", category);
    }

    return 0;
}
