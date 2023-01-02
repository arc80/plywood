/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repo/Common.h>
#include <ply-build-steps/Project.h>
#include <ply-build-repo/Repository.h>
#include <ply-biscuit/Interpreter.h>
#include <ply-build-repo/BuildFolder.h>

namespace ply {
namespace build {

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
void instantiate_all_configs(BuildFolder_t* build_folder);

} // namespace build
} // namespace ply
