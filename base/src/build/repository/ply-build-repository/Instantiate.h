/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>
#include <buildSteps/Project.h>
#include <ply-build-repository/Repository.h>
#include <ply-biscuit/Interpreter.h>
#include <ply-build-folder/BuildFolder.h>

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
    LabelMap<AnyObject> globalNamespace;

    // These members are modified during instantiation.
    LabelMap<TargetWithStatus> moduleMap;
    u64 configBit = 0;
};

MethodResult instantiateModuleForCurrentConfig(Target** target, ModuleInstantiator* mi,
                                               Label moduleLabel);

struct PropertyCollector {
    biscuit::Interpreter* interp;
    String basePath;
    Array<Option>* options;
    u64 configBit = 0;
    bool isModule = false;
};

MethodResult doCustomBlockInsideConfig(PropertyCollector* pc,
                                       const biscuit::Statement::CustomBlock* cb);
void instantiate_all_configs();

} // namespace build2
} // namespace ply
