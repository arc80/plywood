/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/

// This file is meant to be #included from *.modules.cpp files

#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/BuildTarget.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-build-repo/ModuleArgs.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/ProjectInstantiationEnv.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-repo/PackageProvider.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/ExternHelpers.h>
#include <ply-build-provider/HostTools.h>
#include <ply-build-provider/PackageManager.h>
#include <ply-build-provider/ToolchainInfo.h>
#include <ply-runtime/algorithm/Find.h>
using namespace ply;
using namespace ply::build;

#define PLY_DEFINE_MODULE(name, ...) \
    void PLY_UNIQUE_VARIABLE(defineModule_)(__VA_ARGS__)
