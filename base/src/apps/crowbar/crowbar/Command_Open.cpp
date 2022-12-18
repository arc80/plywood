/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/BuildFolder.h>
#include <ply-runtime/io/text/TextConverter.h>

using namespace ply::build;

#if PLY_TARGET_WIN32
#include <combaseapi.h>
#include <shellapi.h>
#endif

bool command_open(CommandLine* cl) {
    ensureTerminated(cl);
    cl->finalize();

    BuildFolder_t bf;
    bf.load(NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crowbar"));

    if (bf.cmakeOptions.generator == "Unix Makefiles") {
        Error.log("No IDE to open for Unix Makefiles");
#if PLY_TARGET_WIN32
    } else if (bf.cmakeOptions.generator.startsWith("Visual Studio")) {
        String slnPath =
            NativePath::join(bf.absPath, "build", bf.solutionName + ".sln");
        if (FileSystem::native()->exists(slnPath) != ExistsResult::File) {
            Error.log("Can't find '{}'", slnPath);
        }

        // Convert to UTF-16 path
        WString wstr;
        {
            MemOutStream mout;
            StringView srcView = slnPath.view();
            TextConverter::create<UTF16_Native, UTF8>().writeTo(&mout, &srcView, true);
            mout << StringView{"\0\0", 2}; // null terminated
            wstr = WString::moveFromString(mout.moveToString());
        }

        // Open IDE
        if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) != S_OK) {
            Error.log("Unable to initialize COM");
            exit(1);
        }
        return ((sptr) ShellExecuteW(NULL, L"open", wstr, NULL, NULL, SW_SHOWNORMAL) > 32);
#endif // PLY_TARGET_WIN32
#if PLY_TARGET_APPLE
    } else if (folder->cmakeOptions.generator == "Xcode") {
        String projPath =
            NativePath::join(folder->getAbsPath(), "build", folder->solutionName + ".xcodeproj");
        if (FileSystem::native()->exists(projPath) != ExistsResult::Directory) {
            fatalError(String::format("Can't find '{}'", projPath));
        }

        // Open IDE
        Owned<Subprocess> sub = Subprocess::exec("open", Array<StringView>{projPath}.view(), {},
                                                 Subprocess::Output::inherit());
        if (!sub) {
            fatalError("Unable to open IDE using 'open'");
        }
        return sub->join() == 0;
#endif // PLY_TARGET_APPLE
    }

    Error.log("Don't know how to open IDE for generator '{}'", bf.cmakeOptions.generator);
    exit(1);
    return false;
}
