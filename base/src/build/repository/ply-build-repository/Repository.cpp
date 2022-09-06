/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Repository.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace build {
namespace latest {

Owned<Repository> Repository::instance;

struct ConfigOptionsInterpreterHooks : crowbar::Interpreter::Hooks {
    crowbar::Interpreter* interp = nullptr;
    Repository::ConfigOptions* optionSet = nullptr;

    virtual ~ConfigOptionsInterpreterHooks() override {
    }

    virtual bool handleLocalAssignment(Label label) override {
        auto cursor = optionSet->map.insertOrFind(label);
        cursor->obj = AnyOwnedObject::create(this->interp->returnValue.type);
        cursor->obj.move(this->interp->returnValue);
        return true;
    }
};

void Repository::create() {
    Repository::instance = Owned<Repository>::create();

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
    for (Module* mod : Repository::instance->moduleMap) {
        mod->defaultOptions = Owned<ConfigOptions>::create();
        if (mod->configBlock) {
            // Create new interpreter.
            MemOutStream outs;
            crowbar::Interpreter interp;
            interp.outs = &outs;

            // Add hooks.
            ConfigOptionsInterpreterHooks interpHooks;
            interpHooks.interp = &interp;
            interpHooks.optionSet = mod->defaultOptions;
            interp.hooks = &interpHooks;

            // Add builtin namespace.
            crowbar::MapNamespace builtIns;
            static bool true_ = true;
            static bool false_ = false;
            builtIns.map.insertOrFind(g_labelStorage.insert("true"))->obj =
                AnyObject::bind(&true_);
            builtIns.map.insertOrFind(g_labelStorage.insert("false"))->obj =
                AnyObject::bind(&false_);
            interp.outerNameSpaces.append(&builtIns);

            // Invoke config_options block.
            crowbar::Interpreter::StackFrame frame;
            frame.interp = &interp;
            frame.desc = {[](Module* mod) -> HybridString {
                              return String::format("config_options for {} '{}'",
                                                    g_labelStorage.view(mod->block->type),
                                                    g_labelStorage.view(mod->block->name));
                          },
                          mod};
            frame.tkr = &mod->plyfile->tkr;
            MethodResult result = execFunction(&frame, mod->configBlock->customBlock()->body);
            if (result == MethodResult::Error) {
                StdErr::text() << outs.moveToString();
                exit(1);
            }
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
