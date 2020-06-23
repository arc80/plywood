/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <CommandHelpers.h>
#include <ConsoleUtils.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-target/BuildTarget.h>

namespace ply {

void AddParams::extractOptions(CommandLine* cl) {
    this->makeShared = cl->checkForSkippedOpt("--shared");
}

bool AddParams::exec(build::BuildFolder* folder, StringView fullTargetName) {
    bool anyChange = false;
    if (findItem(folder->rootTargets.view(), fullTargetName) < 0) {
        folder->rootTargets.append(fullTargetName);
        anyChange = true;
    }
    if (this->makeShared && findItem(folder->makeShared.view(), fullTargetName) < 0) {
        folder->makeShared.append(fullTargetName);
        anyChange = true;
    }
    return anyChange;
}

void BuildParams::extractOptions(CommandLine* cl, const build::BuildFolder* folder) {
    this->configName =
        cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (this->configName) {
        this->configName = this->configName.subStr(9);
    } else {
        this->configName = folder->activeConfig;
    }

    if (cl->checkForSkippedOpt("--add")) {
        this->doAdd = true;
        this->addParams.extractOptions(cl);
    }
}

bool BuildParams::exec(BuildParams::Result* result, build::BuildFolder* folder, bool doBuild) {
    using namespace build;

    // FIXME: Factor this out into a common function with command_generate so that the reason for
    // instantiation failure is logged.
    result->instResult = folder->instantiateAllTargets(false);
    if (!result->instResult.isValid) {
        fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                  folder->buildFolderName));
    }

    if (!this->targetName) {
        this->targetName = folder->activeTarget;
    }

    result->runTargetInst = RepoRegistry::get()->findTargetInstantiator(this->targetName);
    if (!result->runTargetInst) {
        exit(1);
    }

    auto runTargetIdx = result->instResult.dependencyMap.find(result->runTargetInst,
                                                              &result->instResult.dependencies);
    if (!runTargetIdx.wasFound()) {
        if (this->doAdd) {
            this->addParams.exec(folder, result->runTargetInst->getFullyQualifiedName());
            StdOut::createStringWriter().format(
                "Added target '{}' to folder '{}'.\n",
                RepoRegistry::get()->getShortDepSourceName(result->runTargetInst),
                folder->buildFolderName);
            folder->save();

            // Re-instantiate all targets
            result->instResult = folder->instantiateAllTargets(false);
            if (!result->instResult.isValid) {
                fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                          folder->buildFolderName));
            }
            runTargetIdx = result->instResult.dependencyMap.find(result->runTargetInst,
                                                                 &result->instResult.dependencies);
            PLY_ASSERT(runTargetIdx.wasFound());
        } else {
            fatalError(
                String::format("Target '{}' is not instantiated in folder '{}'\n",
                               RepoRegistry::get()->getShortDepSourceName(result->runTargetInst),
                               folder->buildFolderName));
        }
    }

    result->runTarget =
        static_cast<BuildTarget*>(result->instResult.dependencies[*runTargetIdx].dep.get());
    PLY_ASSERT(result->runTarget->type == DependencyType::Target);
    if (result->runTarget->targetType != BuildTargetType::EXE) {
        fatalError(String::format("Target '{}' is not executable\n", this->targetName));
    }

    // Generate & build
    if (doBuild) {
        if (!folder->isGenerated(this->configName)) {
            if (!folder->generateLoop(this->configName))
                return false;
        }
        if (!folder->build(this->configName, result->runTarget->name, false))
            return false;
    }

    return true;
}

} // namespace ply
