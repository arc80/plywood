/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <WorkspaceSettings.h>

namespace ply {

namespace build {
struct BuildFolder;
} // namespace build

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;
    Array<StringView> skippedOpts;
    bool finalized = false;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>{(const char**) argv, (u32) argc}} {
    }
    ~CommandLine() {
        PLY_ASSERT(this->finalized);
    }

    StringView readToken();
    bool checkForSkippedOpt(StringView arg);
    void finalize();
};

struct PlyToolCommandEnv {
    CommandLine* cl = nullptr;
    WorkspaceSettings* workspace = nullptr;
    Array<Owned<build::BuildFolder>> buildFolders;
    build::BuildFolder* currentBuildFolder = nullptr;
};

bool prefixMatch(StringView input, StringView cmd, u32 minUnits = 2);
void fatalError(StringView msg);
void ensureTerminated(CommandLine* cl);

struct CommandDescription {
    StringView name;
    StringView description;
};

using CommandList = ArrayView<const CommandDescription>;

void printUsage(StringWriter* sw, CommandList commands);
void printUsage(StringWriter* sw, StringView command, CommandList commands = {});

} // namespace ply
