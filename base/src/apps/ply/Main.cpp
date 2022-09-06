/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Instantiate.h>

using namespace ply;

bool test() {
    build::latest::Repository::create();

    String buildFolderPath = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crap");
    build::latest::ModuleInstantiator mi{buildFolderPath};
    mi.project.name = "Test";

    // Add configs
    mi.project.configs.append("Debug");

    // For each config, instantiate root modules
    for (u32 i = 0; i < mi.project.configs.numItems(); i++) {
        mi.currentConfig = i;
        buildSteps::Node* node =
            instantiateModuleForCurrentConfig(&mi, g_labelStorage.find("HelloWorld"));
        if (!node)
            return false;
        mi.project.rootNodes.append(node);
    }

    Owned<buildSteps::MetaProject> mp = expand(&mi.project);
    MemOutStream outs;
    writeCMakeLists(&outs, mp);

    String outPath = NativePath::join(buildFolderPath, "CMakeLists.txt");
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(outPath, outs.moveToString(),
                                                         TextFormat::platformPreference());

    return true;
}

int main() {
    return test() ? 0 : 1;
}

//#include "codegen/Main.inl"
