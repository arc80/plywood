/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Repository.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace build {
namespace latest {

Repository* g_repository = nullptr;

struct ConfigOptionsInterpreter {
    crowbar::Interpreter interp;
    Repository::ConfigOptions* optionSet = nullptr;
};

bool doLocalAssignment(ConfigOptionsInterpreter* coi, Label label) {
    auto cursor = coi->optionSet->map.insertOrFind(label);
    cursor->obj = AnyOwnedObject::create(coi->interp.base.returnValue.type);
    cursor->obj.move(coi->interp.base.returnValue);
    return true;
}

void Repository::create() {
    g_repository = new Repository;

    bool anyError = false;
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(PLY_WORKSPACE_FOLDER, 0)) {
        if (!entry.isDir)
            continue;
        if (entry.name.startsWith("."))
            continue;
        if (entry.name == "data")
            continue;

        // Recursively find all Plyfiles
        String repoFolder = NativePath::join(PLY_WORKSPACE_FOLDER, entry.name);
        for (const WalkTriple& triple : FileSystem::native()->walk(repoFolder)) {
            for (const WalkTriple::FileInfo& file : triple.files) {
                if (file.name == "Plyfile") {
                    if (!parsePlyfile(NativePath::join(triple.dirPath, file.name))) {
                        anyError = true;
                    }
                }
            }
        }
    }

    if (anyError) {
        exit(1);
    }

    // Initialize all config_options
    for (Module* mod : g_repository->moduleMap) {
        mod->defaultOptions = Owned<ConfigOptions>::create();
    }

    for (const ModuleConfigBlock& cb : g_repository->moduleConfigBlocks) {
        // Create new interpreter.
        ConfigOptionsInterpreter coi;
        coi.interp.base.error = [](StringView message) { StdErr::text() << message; };
        coi.optionSet = cb.mod->defaultOptions;
        coi.interp.hooks.assignToLocal = {doLocalAssignment, &coi};

        // Add builtin namespace.
        static bool true_ = true;
        static bool false_ = false;
        *coi.interp.builtIns.insert(g_labelStorage.insert("true")) = AnyObject::bind(&true_);
        *coi.interp.builtIns.insert(g_labelStorage.insert("false")) = AnyObject::bind(&false_);

        // Invoke config_options block.
        crowbar::Interpreter::StackFrame frame;
        frame.interp = &coi.interp;
        frame.desc = [mod = cb.mod]() -> HybridString {
            return String::format("config_options for {} '{}'",
                                  g_labelStorage.view(mod->block->type),
                                  g_labelStorage.view(mod->block->name));
        };
        frame.tkr = &cb.mod->plyfile->tkr;
        MethodResult result = execFunction(&frame, cb.block->customBlock()->body);
        if (result == MethodResult::Error) {
            exit(1);
        }
    }
}

PLY_NO_INLINE MethodTable getMethodTable_Repository_ConfigOptions() {
    MethodTable methods;
    methods.propertyLookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                StringView propertyName) -> MethodResult {
        Label label = g_labelStorage.find(propertyName);
        if (label) {
            auto* configOpts = obj.cast<Repository::ConfigOptions>();
            auto cursor = configOpts->map.find(label);
            if (cursor.wasFound()) {
                interp->returnValue = cursor->obj;
                return MethodResult::OK;
            }
        }

        interp->returnValue = {};
        interp->error(
            String::format("configuration option '{}' not found in module", propertyName));
        return MethodResult::Error;
    };
    return methods;
}

TypeKey TypeKey_Repository_ConfigOptions{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "ConfigOptions";
    },

    // write
    nullptr, // Unimplemented

    // writeFormat
    nullptr, // Unimplemented

    // read
    nullptr, // Unimplemented

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace latest
} // namespace build

PLY_DEFINE_TYPE_DESCRIPTOR(build::latest::Repository::ConfigOptions) {
    static TypeDescriptor typeDesc{
        &build::latest::TypeKey_Repository_ConfigOptions,
        (build::latest::Repository::ConfigOptions*) nullptr,
        NativeBindings::make<build::latest::Repository::ConfigOptions>()
            PLY_METHOD_TABLES_ONLY(, build::latest::getMethodTable_Repository_ConfigOptions())};
    return &typeDesc;
}

} // namespace ply

#include "codegen/Repository.inl"
