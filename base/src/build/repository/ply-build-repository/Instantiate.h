/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>
#include <buildSteps/Project.h>
#include <ply-build-repository/Repository.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace build2 {

enum Status {
    NotInstantiated,
    Instantiating,
    Instantiated,
};

struct TargetWithStatus {
    Target* target = nullptr;
    Status statusInCurrentConfig = NotInstantiated;
};

struct ModuleInstantiator {
    String buildFolderPath;
    LabelMap<AnyObject> globalNamespace;

    // These members are modified during instantiation.
    LabelMap<TargetWithStatus> moduleMap;
    u64 configBit = 0;

    ModuleInstantiator(StringView buildFolderPath) : buildFolderPath{buildFolderPath} {
    }
};

MethodResult instantiateModuleForCurrentConfig(Target** target, ModuleInstantiator* mi,
                                               Label moduleLabel);

struct PropertyCollector {
    crowbar::Interpreter* interp;
    String basePath;
    Array<Option>* options;
    u64 configBit = 0;
    bool isModule = false;
};

MethodResult doCustomBlockInsideConfig(PropertyCollector* pc,
                                       const crowbar::Statement::CustomBlock* cb);

} // namespace build2
} // namespace ply
