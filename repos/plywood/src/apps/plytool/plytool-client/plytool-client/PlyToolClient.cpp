/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <plytool-client/Core.h>
#include <plytool-client/PlyToolClient.h>
#include <plytool-client/Command.h>
#include <ply-reflect/PersistWrite.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/io/impl/Pipe_Win32.h>

namespace ply {

PLY_NO_INLINE void PlyToolClient::remoteRun(ArrayView<String> sourceFiles,
                                            ArrayView<tool::Command::Dependency> dependencies) {
    // FIXME: Don't hardcode this path
    String exePath = NativePath::join(PLY_WORKSPACE_FOLDER,
                                      "data/build/bootstrap/PlyTool/build/Debug/PlyTool.exe");
    Owned<Subprocess> plyTool =
        Subprocess::exec(exePath, {"rpc"}, {}, Subprocess::Output::inherit());
    OutStream outs{plyTool->writeToStdIn.borrow()};

    WriteFormatContext writeFormatContext{&outs};
    writeFormatContext.addOrGetFormatID(getTypeDescriptor<tool::Command>());
    writeFormatContext.endSchema();
    outs.flushMem();

    // Command to send
    tool::Command cmd;
    auto run = cmd.type.run().switchTo();
    run->sourceFiles = sourceFiles;
    run->dependencies = dependencies;

    // Serialize and send to CookEXE over m_subprocess.stdinWr
    WriteObjectContext writeObjectContext{&outs, &writeFormatContext};
    TypedPtr obj = TypedPtr::bind(&cmd);

    // Write formatID (FIXME: This seems redundant)
    u32 formatID = writeFormatContext.getFormatID(obj.type);
    writeObjectContext.out.write<u32>(formatID);

    // Write the object
    obj.type->typeKey->write(obj, &writeObjectContext);
    outs.flushMem();

    OutPipe_Win32* winPipe = plyTool->writeToStdIn->cast<OutPipe_Win32>();
    BOOL rch = CloseHandle(winPipe->handle);
    PLY_ASSERT(rch != 0);
    winPipe->handle = INVALID_HANDLE_VALUE;
    u32 rc = plyTool->join();
    PLY_ASSERT(rc == 0);
}

} // namespace ply

#endif // PLY_TARGET_WIN32
