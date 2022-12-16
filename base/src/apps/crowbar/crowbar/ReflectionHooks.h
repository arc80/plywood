/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-runtime/io/text/TextFormat.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/algorithm/Sort.h>

namespace ply {
namespace cpp {

struct Subst {
    u32 start = 0;
    u32 numBytes = 0;
    String replacement;

    PLY_INLINE bool operator<(const Subst& other) const {
        return (this->start < other.start) ||
               (this->start == other.start && this->numBytes < other.numBytes);
    }
};

struct ReflectedClass {
    PLY_REFLECT()
    String cppInlPath;
    String name;
    Array<String> members;
    // ply reflect off
};

// FIXME: The parser actually fills in Enum_::enumerators, making this struct redundant.
// Find a way to simplify.
struct ReflectedEnum {
    PLY_REFLECT()
    String cppInlPath;
    String namespacePrefix;
    String enumName;
    Array<String> enumerators;
    // ply reflect off
};

struct SwitchInfo {
    PLY_REFLECT()
    Token macro;
    String inlineInlPath;
    String cppInlPath;
    String name;
    bool isReflected = false;
    Array<String> states;
    // ply reflect off
};

struct ReflectionInfoAggregator {
    Array<Owned<ReflectedClass>> classes;
    Array<Owned<ReflectedEnum>> enums;
    Array<Owned<SwitchInfo>> switches;
};

struct SingleFileReflectionInfo {
    // May later need to generalize to multiple files
    Array<Subst> substsInParsedFile;
    Array<SwitchInfo*> switches;
};

Tuple<SingleFileReflectionInfo, bool> extractReflection(cpp::ReflectionInfoAggregator* agg,
                                                        StringView relPath);

} // namespace cpp
} // namespace ply
