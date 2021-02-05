/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-runtime/io/text/TextFormat.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/algorithm/Sort.h>
#include <ReflectionHooks.h>
#include <ConsoleUtils.h>

namespace ply {
namespace cpp {

struct CodeGenerator {
    virtual ~CodeGenerator() {
    }
    virtual void write(OutStream* outs) = 0;
};

String getSwitchInl(SwitchInfo* switch_) {
    MemOutStream mout;
    mout << "enum class ID : u16 {\n";
    for (StringView state : switch_->states) {
        mout.format("    {},\n", state);
    }
    mout << "    Count,\n";
    mout << "};\n";
    mout << "union Storage_ {\n";
    for (StringView state : switch_->states) {
        mout.format("    {} {}{};\n", state, state.left(1).lowerAsc(), state.subStr(1));
    }
    mout << "    PLY_INLINE Storage_() {}\n";
    mout << "    PLY_INLINE ~Storage_() {}\n";
    mout << "};\n";
    StringView className = switch_->name.splitByte(':').back(); // FIXME: more elegant
    // FIXME: Log an error if there are no states
    mout.format("SWITCH_FOOTER({}, {})\n", className, switch_->states[0]);
    for (StringView state : switch_->states) {
        mout.format("SWITCH_ACCESSOR({}, {}{})\n", state, state.left(1).lowerAsc(),
                    state.subStr(1));
    }
    if (switch_->isReflected) {
        mout << "PLY_SWITCH_REFLECT()\n";
    }
    return mout.moveToString();
}

void writeSwitchInl(SwitchInfo* switch_, const TextFormat& tff) {
    String absInlPath = NativePath::join(PLY_WORKSPACE_FOLDER, "repos", switch_->inlineInlPath);
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        absInlPath, getSwitchInl(switch_), tff);
    OutStream stdOut = StdOut::text();
    if (result == FSResult::OK) {
        stdOut.format("Wrote {}\n", absInlPath);
    } else if (result != FSResult::Unchanged) {
        stdOut.format("Error writing {}\n", absInlPath);
    }
}

String performSubsts(StringView absPath, ArrayView<Subst> substs) {
    String src = FileSystem::native()->loadTextAutodetect(absPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return {};

    MemOutStream mout;
    u32 prevEndPos = 0;
    for (const Subst& subst : substs) {
        PLY_ASSERT(subst.start >= prevEndPos);
        u32 endPos = subst.start + subst.numBytes;
        PLY_ASSERT(endPos < src.numBytes);
        mout.write({src.bytes + prevEndPos, subst.start - prevEndPos});
        mout.write(subst.replacement);
        prevEndPos = endPos;
    }
    mout.write({src.bytes + prevEndPos, src.numBytes - prevEndPos});
    return mout.moveToString();
}

void performSubstsAndSave(StringView absPath, ArrayView<Subst> substs, const TextFormat& tff) {
    // FIXME: Don't reload the file here!!!!!!!!!!
    // It may have changed, making the Substs invalid!!!!!!!!
    String srcWithSubst = performSubsts(absPath, substs);
    if (FileSystem::native()->lastResult() == FSResult::OK) {
        FSResult result =
            FileSystem::native()->makeDirsAndSaveTextIfDifferent(absPath, srcWithSubst, tff);
        OutStream stdOut = StdOut::text();
        if (result == FSResult::OK) {
            stdOut.format("Wrote {}\n", absPath);
        } else if (result != FSResult::Unchanged) {
            stdOut.format("Error writing {}\n", absPath);
        }
    }
}

} // namespace cpp

void generateAllCppInls(cpp::ReflectionInfoAggregator* agg, const TextFormat& tff) {
    struct CodeGenerator {
        virtual ~CodeGenerator() {
        }
        virtual void write(OutStream* outs) = 0;
    };

    struct Traits {
        using Key = StringView;
        struct Item {
            String cppInlPath;
            Array<Owned<CodeGenerator>> sources;
            Item(const Key& key) : cppInlPath{key} {
            }
        };
        static PLY_INLINE bool match(const Item& item, Key key) {
            return item.cppInlPath == key;
        }
    };
    HashMap<Traits> fileToGeneratorList;

    for (cpp::ReflectedClass* clazz : agg->classes) {
        struct StructGenerator : CodeGenerator {
            cpp::ReflectedClass* clazz;
            virtual void write(OutStream* outs) override {
                outs->format("PLY_STRUCT_BEGIN({})\n", this->clazz->name);
                for (StringView member : this->clazz->members) {
                    outs->format("PLY_STRUCT_MEMBER({})\n", member);
                }
                *outs << "PLY_STRUCT_END()\n\n";
            }
            StructGenerator(cpp::ReflectedClass* clazz) : clazz{clazz} {
            }
        };
        auto cursor = fileToGeneratorList.insertOrFind(clazz->cppInlPath);
        cursor->sources.append(new StructGenerator{clazz});
    }

    for (cpp::ReflectedEnum* enum_ : agg->enums) {
        struct EnumGenerator : CodeGenerator {
            cpp::ReflectedEnum* enum_;
            virtual void write(OutStream* outs) override {
                outs->format("PLY_ENUM_BEGIN({}, {})\n", this->enum_->namespacePrefix,
                             this->enum_->enumName);
                for (u32 i : range(this->enum_->enumerators.numItems())) {
                    StringView enumerator = this->enum_->enumerators[i];
                    if ((i != this->enum_->enumerators.numItems() - 1) || (enumerator != "Count")) {
                        outs->format("PLY_ENUM_IDENTIFIER({})\n", enumerator);
                    }
                }
                outs->format("PLY_ENUM_END()\n\n");
            }
            EnumGenerator(cpp::ReflectedEnum* enum_) : enum_{enum_} {
            }
        };
        auto cursor = fileToGeneratorList.insertOrFind(enum_->cppInlPath);
        cursor->sources.append(new EnumGenerator{enum_});
    }

    for (cpp::SwitchInfo* switch_ : agg->switches) {
        struct SwitchGenerator : CodeGenerator {
            cpp::SwitchInfo* switch_;
            virtual void write(OutStream* outs) override {
                outs->format("SWITCH_TABLE_BEGIN({})\n", this->switch_->name);
                for (StringView state : this->switch_->states) {
                    outs->format("SWITCH_TABLE_STATE({}, {})\n", this->switch_->name, state);
                }
                outs->format("SWITCH_TABLE_END({})\n\n", this->switch_->name);

                if (this->switch_->isReflected) {
                    outs->format("PLY_SWITCH_BEGIN({})\n", this->switch_->name);
                    for (StringView state : this->switch_->states) {
                        outs->format("PLY_SWITCH_MEMBER({})\n", state);
                    }
                    outs->format("PLY_SWITCH_END()\n\n");
                }
            }
            SwitchGenerator(cpp::SwitchInfo* switch_) : switch_{switch_} {
            }
        };
        auto cursor = fileToGeneratorList.insertOrFind(switch_->cppInlPath);
        cursor->sources.append(new SwitchGenerator{switch_});
    }

    for (const Traits::Item& item : fileToGeneratorList) {
        PLY_ASSERT(item.cppInlPath.endsWith(".inl"));
        String absPath = NativePath::join(PLY_WORKSPACE_FOLDER, "repos", item.cppInlPath);

        MemOutStream mout;
        for (CodeGenerator* generator : item.sources) {
            generator->write(&mout);
        }
        FSResult result =
            FileSystem::native()->makeDirsAndSaveTextIfDifferent(absPath, mout.moveToString(), tff);
        OutStream stdOut = StdOut::text();
        if (result == FSResult::OK) {
            stdOut.format("Wrote {}\n", absPath);
        } else if (result != FSResult::Unchanged) {
            stdOut.format("Error writing {}\n", absPath);
        }
    }
}

void command_codegen(PlyToolCommandEnv* env) {
    ensureTerminated(env->cl);
    env->cl->finalize();

    cpp::ReflectionInfoAggregator agg;

    u32 fileNum = 0;
    for (WalkTriple& triple :
         FileSystem::native()->walk(NativePath::join(PLY_WORKSPACE_FOLDER, "repos"))) {
        // Sort child directories and filenames so that files are visited in a deterministic order:
        sort(triple.dirNames.view());
        sort(triple.files.view(), [](const WalkTriple::FileInfo& a, const WalkTriple::FileInfo& b) {
            return a.name < b.name;
        });

        if (find(triple.files.view(),
                 [](const auto& fileInfo) { return fileInfo.name == "nocodegen"; }) >= 0) {
            triple.dirNames.clear();
            continue;
        }

        for (const WalkTriple::FileInfo& file : triple.files) {
            if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                if (file.name.endsWith(".modules.cpp"))
                    continue;
                // FIXME: Eliminate exclusions
                for (StringView exclude : {
                         "Sort.h",
                         "Functor.h",
                         "DirectoryWatcher_Mac.h",
                         "DirectoryWatcher_Win32.h",
                         "Heap.cpp",
                         "HiddenArgFunctor.h",
                         "LambdaView.h",
                         "Pool.h",
                     }) {
                    if (file.name == exclude)
                        goto skipIt;
                }
                {
                    fileNum++;

                    Tuple<cpp::SingleFileReflectionInfo, bool> sfri = cpp::extractReflection(
                        &agg, NativePath::join(triple.dirPath, file.name).view());
                    if (sfri.second) {
                        for (cpp::SwitchInfo* switch_ : sfri.first.switches) {
                            writeSwitchInl(switch_, env->workspace->getSourceTextFormat());
                        }
                        performSubstsAndSave(NativePath::join(triple.dirPath, file.name),
                                             sfri.first.substsInParsedFile.view(),
                                             env->workspace->getSourceTextFormat());
                    }
                }
            skipIt:;
            }
        }
        for (StringView exclude : {"Shell_iOS", "opengl-support"}) {
            s32 i = findItem(triple.dirNames.view(), exclude);
            if (i >= 0) {
                triple.dirNames.erase(i);
            }
        }
    }

    generateAllCppInls(&agg, env->workspace->getSourceTextFormat());
}

} // namespace ply
