/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>
#include <buildSteps/buildSteps.h>
#include <ply-build-repository/Repository.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace build {
namespace latest {

struct ModuleInstantiator {
    enum Status { NotInstantiated, Instantiating, Instantiated };

    struct ModuleMapTraits {
        using Key = StringView;
        struct Item {
            Reference<buildSteps::Node> node;
            Status statusInCurrentConfig = NotInstantiated;
        };
        static bool match(const Item& item, StringView name) {
            return item.node->name == name;
        }
    };

    String buildFolderPath;
    MemOutStream errorOut;
    Owned<crowbar::INamespace> globalNamespace;

    // The project is initialized by instantiating a set of root modules.
    buildSteps::Project project;

    // These members are only used while the project is being instantiated.
    HashMap<ModuleMapTraits> modules;
    u32 currentConfig = 0;

    ModuleInstantiator(StringView buildFolderPath);
};

buildSteps::Node* instantiateModuleForCurrentConfig(ModuleInstantiator* mi, Label moduleLabel);

} // namespace latest
} // namespace build
} // namespace ply
