/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "core.h"
#include "command_line.h"
#include "workspace.h"
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
    tidy_repo(Path.join(Workspace.path, "base/src"), {},
              Workspace.getSourceTextFormat());
}

//   ▄▄                                ▄▄
//  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄  ▄██▄▄
//   ██    ▄▄▄██ ██  ▀▀ ██  ██ ██▄▄██  ██
//   ▀█▄▄ ▀█▄▄██ ██     ▀█▄▄██ ▀█▄▄▄   ▀█▄▄
//                       ▄▄▄█▀

void cmd_target(CommandLine& cl) {
    StringView cmd = cl.next_arg();
    if (cmd.isEmpty())
        goto print_usage;

    if (prefix_match(cmd, "add")) {
        StringView targetName = cl.next_arg();
        if (targetName.isEmpty()) {
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
    String path = Path.join(Workspace.path, "data/build", Workspace.currentBuildFolder);
    bf.load(path);
    generate(&bf);
}

//  ▄▄                    ▄▄           ▄▄
//  ██▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██  ██  ██   ▀█▄▄▄   ██   ██  ▀▀  ▄▄▄██ ██  ██
//  ██▄▄█▀ ▀█▄▄█▀ ▀█▄▄█▀  ▀█▄▄  ▄▄▄█▀  ▀█▄▄ ██     ▀█▄▄██ ██▄▄█▀
//                                                        ██

void cmd_bootstrap(CommandLine& cl) {
    StringView configName;
    cl.find_option("config", &configName);
    cl.check_for_unused_args();

    Common::initialize();
    Repository::create();

    BuildFolder_t bf;
    bf.absPath = Path.join(Workspace.path, "data/build/crowbar");
    bf.solutionName = "crowbar";
    bf.rootTargets.append("crowbar");

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
    String path = Path.join(Workspace.path, "data/build", Workspace.currentBuildFolder);
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
    MemOutStream outs;
    if (StringView word = cl.next_arg()) {
        outs << word;
    }
    while (StringView word = cl.next_arg()) {
        outs << " " << word;
    }
    cl.check_for_unused_args();

    print_bigfont(outs.moveToString());
}

//    ▄▄▄        ▄▄▄      ▄▄
//   ██    ▄▄▄▄   ██   ▄▄▄██  ▄▄▄▄  ▄▄▄▄▄
//  ▀██▀  ██  ██  ██  ██  ██ ██▄▄██ ██  ▀▀
//   ██   ▀█▄▄█▀ ▄██▄ ▀█▄▄██ ▀█▄▄▄  ██
//

void cmd_folder(CommandLine& cl) {
    StringView cmd = cl.next_arg();
    if (cmd.isEmpty())
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
            StdOut::text().format("Current build folder is now '{}'\n", name);
        }
        return;

    } else if (prefix_match(cmd, "list")) {
        // list
        cl.check_for_unused_args();
        Array<String> folder_names = get_build_folders();
        OutStream outs = StdOut::text();
        for (StringView name : folder_names) {
            outs << name;
            if (name == Workspace.currentBuildFolder) {
                outs << " (current)";
            }
            outs << '\n';
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
            StdOut::text().format("Current build folder is now '{}'\n", name);
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
    } else if (prefix_match(category, "folder")) {
        cmd_folder(cl);
    } else {
        Error.log("Unrecognized command \"{}\"", category);
    }

    return 0;
}
