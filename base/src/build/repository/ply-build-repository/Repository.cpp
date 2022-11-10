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

bool doLocalAssignment(ConfigOptionsInterpreter* coi, const AnyObject& attributes, Label label) {
    PLY_ASSERT(!attributes.data);
    AnyOwnedObject* obj;
    coi->optionSet->map.insertOrFind(label, &obj);
    *obj = AnyOwnedObject::create(coi->interp.base.returnValue.type);
    obj->move(coi->interp.base.returnValue);
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
    for (ModuleOrFunction* mod : g_repository->modules) {
        mod->defaultOptions = Owned<ConfigOptions>::create();
    }

    for (const ModuleConfigBlock& cb : g_repository->moduleConfigBlocks) {
        // Create new interpreter.
        ConfigOptionsInterpreter coi;
        coi.interp.base.error = [&coi](StringView message) {
            OutStream outs = StdErr::text();
            logErrorWithStack(&outs, &coi.interp, message);
        };
        coi.optionSet = cb.mod->defaultOptions;

        // Add builtin namespace.
        LabelMap<AnyObject> builtIns;
        static bool true_ = true;
        static bool false_ = false;
        *builtIns.insert(g_labelStorage.insert("true")) = AnyObject::bind(&true_);
        *builtIns.insert(g_labelStorage.insert("false")) = AnyObject::bind(&false_);
        coi.interp.resolveName = [&builtIns](Label identifier) -> AnyObject {
            if (AnyObject* builtIn = builtIns.find(identifier))
                return *builtIn;
            return {};
        };

        // Invoke config_options block.
        crowbar::Interpreter::StackFrame frame;
        frame.interp = &coi.interp;
        frame.hooks.assignToLocal = {doLocalAssignment, &coi};
        frame.desc = [mod = cb.mod]() -> HybridString {
            return String::format("config_options for {} '{}'",
                                  g_labelStorage.view(mod->stmt->customBlock()->type),
                                  g_labelStorage.view(mod->stmt->customBlock()->name));
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
            AnyOwnedObject* prop;
            configOpts->map.insertOrFind(label, &prop);
            interp->returnValue = *prop;
            return MethodResult::OK;
        }
        // read-only access?

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
