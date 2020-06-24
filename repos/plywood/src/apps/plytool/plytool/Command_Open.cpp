/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-runtime/io/text/TextConverter.h>

#if PLY_TARGET_WIN32
#include <combaseapi.h>
#include <shellapi.h>
#endif

namespace ply {

bool command_open(PlyToolCommandEnv* env) {
    using namespace build;

    ensureTerminated(env->cl);
    env->cl->finalize();

    const BuildFolder* folder = env->currentBuildFolder;
    if (!folder) {
        fatalError("Current build folder not set");
    }

    if (folder->cmakeOptions.generator == "Unix Makefiles") {
        fatalError("No IDE to open for Unix Makefiles");
#if PLY_TARGET_WIN32
    } else if (folder->cmakeOptions.generator.startsWith("Visual Studio")) {
        String slnPath =
            NativePath::join(folder->getAbsPath(), "build", folder->solutionName + ".sln");
        if (FileSystem::native()->exists(slnPath) != ExistsResult::File) {
            fatalError(String::format("Can't find '{}'", slnPath));
        }

        // Convert to UTF-16 path
        WString wstr;
        {
            MemOutStream mout;
            ConstBufferView srcView = slnPath.view().bufferView();
            TextConverter::create<UTF16_Native, UTF8>().writeTo(&mout, &srcView, true);
            *mout.strWriter() << "\0\0"; // null terminated
            wstr = WString::moveFromBuffer(mout.moveToBuffer());
        }

        // Open IDE
        if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) != S_OK) {
            fatalError("Unable to initialize COM");
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

    fatalError(String::format("Don't know how to open IDE for generator '{}'",
                              folder->cmakeOptions.generator));
    return false;
}

} // namespace ply
