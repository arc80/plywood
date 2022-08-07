<% title "Adding New Extern Providers" %>

In Plywood, [extern providers](KeyConcepts#extern-providers) are defined by adding a special C++ function to a file with the suffix `.modules.cpp` somewhere in a [repo](KeyConcepts#repos)'s directory tree. This function is called an **extern function** and must be preceded by a line comment of the form:

    // [ply extern="<extern-name>" provider="<provider-name>"]

_<extern-name>_ can be prefixed with the name of a repo. If _<extern-name>_ refers to the current repo, a new [extern](KeyConcepts#externs) with that name is implicitly created and added to the current repo. If _<extern-name>_ was already defined by another repo that the current repo depends on, a new provider is added to the set of available providers for that extern.

For example, one of Plywood's built-in extern providers defined in [`repos/plywood/src/web/web.modules.cpp`](https://github.com/arc80/plywood/blob/main/repos/plywood/src/web/web.modules.cpp) uses the extern name `"libsass"` and provider name `"prebuilt"`:

    // [ply extern="libsass" provider="prebuilt"]
    ExternResult extern_libsass_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
        // Toolchain filters
        if (args->toolchain->targetPlatform.name != "windows") {
            return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
        }
        if (find<StringView>({"x86", "x64"}, args->toolchain->arch) < 0) {
            return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
        }
        if (args->providerArgs) {
            return {ExternResult::BadArgs, ""};
        }
        ...

The name of the function itself (in this case, `extern_libsass_prebuilt`) isn't really important, except that it must be unique across all `.modules.cpp` files in the current repo. By convention, the function name usually starts with `extern_`.

The set of valid operations that can be performed by an extern provider is not yet documented. For the time being, you can study existing examples in the `plywood` repo.
