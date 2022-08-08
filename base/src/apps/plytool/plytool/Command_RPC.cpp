/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/Asset.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/ProjectInstantiationEnv.h>
#include <ply-build-repo/ModuleArgs.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <plytool-client/Command.h>
#include <ConsoleUtils.h>

namespace ply {

PLY_NO_INLINE void buildAndRun(const tool::Command::Type::Run& runCmd) {
    using namespace build;
    WorkspaceSettings workspace;
    if (!workspace.load()) {
        PLY_ASSERT(0);
        // fatalError(String::format("Invalid workspace settings file: \"{}\"",
        //                          WorkspaceSettings::getPath()));
    }

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    TargetInstantiator targetInst;
    targetInst.name = "AutoDiagramMaker"; // FIXME: generalize
    targetInst.instantiatorPath = NativePath::join(PLY_WORKSPACE_FOLDER, "data/docsite-cache");
    targetInst.moduleFunc = [&runCmd](ModuleArgs* args) {
        args->buildTarget->targetType = BuildTargetType::EXE;
        args->addSourceFiles(runCmd.sourceFiles[0]);
        for (const tool::Command::Dependency& dep : runCmd.dependencies) {
            // FIXME: dep.repo is unused
            PLY_ASSERT(dep.depType == tool::Command::Dependency::Type::Target);
            args->addTarget(Visibility::Private, dep.depName);
        }
    };

    // FIXME: Don't require a BuildFolder object, and let the runCmd specify the output path
    Owned<BuildFolder> info = BuildFolder::create("AutoDiagramMaker", "AutoDiagramMaker");
    Owned<ProjectInstantiationEnv> instEnv = info->createEnvironment();
    // FIXME: Set config
    // FIXME: Don't hardcode
    instEnv->externSelectors = {
        RepoRegistry::get()->getExternProvider("ply.cairo.prebuiltDLLs"),
        RepoRegistry::get()->getExternProvider("ply.libpng.builtFromSource"),
    };
    ProjectInstantiationResult instResult;
    ProjectInstantiator projInst;
    projInst.env = instEnv;
    projInst.result = &instResult;
    projInst.instantiate(&targetInst, false);
    projInst.propagateAllDependencies();
    // FIXME: Notify parent process of unselected/uninstalled providers:
    if (instResult.unselectedExterns.isEmpty() && instResult.uninstalledProviders.isEmpty()) {
        info->generate({}, &instResult);
        info->build({}, {}, true);
    }
}

PLY_NO_INLINE void command_rpc(PlyToolCommandEnv*) {
    InStream ins = StdIn::binary();
    RegistryTypeResolver resolver;
    resolver.add(getTypeDescriptor<tool::Command>());
    Schema schema;
    readSchema(schema, &ins);

    for (;;) {
        ReadObjectContext context{&schema, &ins, &resolver};
        AnyOwnedObject obj = readObject(&context);
        if (!obj.data)
            break; // Pipe was closed

        tool::Command* cmd = obj.cast<tool::Command>();
        switch (cmd->type.id) {
            case tool::Command::Type::ID::Run: {
                buildAndRun(*cmd->type.run().get());
                break;
            }

            default: {
                PLY_ASSERT(0);
                break;
            }
        }
    }
}

} // namespace ply
