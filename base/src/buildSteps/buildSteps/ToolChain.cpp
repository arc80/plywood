/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/ToolChain.h>
#include <ply-runtime/algorithm/Find.h>

namespace buildSteps {

struct ToolChain {
    struct PossibleValueToAdd {
        StringView name;
        ArrayView<const StringView> compileFlags;
        ArrayView<const StringView> linkFlags;
    };

    struct PossibleValue {
        String name;
        Array<String> compileFlags;
        Array<String> linkFlags;

        PossibleValue(const PossibleValueToAdd& from)
            : name{from.name}, compileFlags{from.compileFlags}, linkFlags{from.linkFlags} {
        }
    };

    struct Traits {
        using Key = StringView;
        struct Item {
            StringView name;
            Array<PossibleValue> possibleValues;
            Item(StringView name) : name{name} {
            }
        };
        static bool match(const Item& item, StringView name) {
            return item.name == name;
        }
    };

    HashMap<Traits> genericOptionMap;

    void addOption(StringView key, ArrayView<const PossibleValueToAdd> possibleValues) {
        auto cursor = this->genericOptionMap.insertOrFind(key);
        PLY_ASSERT(!cursor.wasFound());
        cursor->possibleValues = possibleValues;
    }
};

Owned<ToolChain> getMSVC() {
    Owned<ToolChain> tc = Owned<ToolChain>::create();
    tc->addOption("DebugInfo", {{{}, {"/Zi"}, {"/DEBUG"}}});
    tc->addOption("LanguageLevel", {{"c++14", {"/std:c++14"}}, {"c++17", {"/std:c++17"}}});
    return tc;
}

Owned<ToolChain> getGCC() {
    Owned<ToolChain> tc = Owned<ToolChain>::create();
    tc->addOption("DebugInfo", {{{}, {"-g"}}});
    tc->addOption("LanguageLevel", {{"c++14", {"-std=c++14"}}, {"c++17", {"-std=c++17"}}});
    return tc;
}

void translateGenericOption(ToolChain* tc, CompileOpts* copts, StringView genericKey, StringView genericValue) {
    auto cursor = tc->genericOptionMap.find(genericKey);
    PLY_ASSERT(cursor.wasFound());
    s32 i = 0;
    if (genericValue) {
        i = find(cursor->possibleValues, [&](const auto pv) { return pv.name == genericValue; });
        PLY_ASSERT(i >= 0);
    } else {
        PLY_ASSERT(cursor->possibleValues.numItems() == 1);
    }
    copts->compileFlags.extend(cursor->possibleValues[i].compileFlags);
    copts->linkFlags.extend(cursor->possibleValues[i].linkFlags);
}

void destroy(ToolChain* tc) {
    delete tc;
}

} // namespace buildSteps
