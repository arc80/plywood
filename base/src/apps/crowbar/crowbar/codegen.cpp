/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include "core.h"
#include "workspace.h"
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-cpp/ErrorFormatting.h>

//                              ▄▄
//  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//  ██  ██  ▄▄▄██ ██  ▀▀ ▀█▄▄▄  ██ ██  ██ ██  ██
//  ██▄▄█▀ ▀█▄▄██ ██      ▄▄▄█▀ ██ ██  ██ ▀█▄▄██
//  ██                                     ▄▄▄█▀

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
    cpp::Token macro;
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

namespace ply {
namespace cpp {
void parsePlywoodSrcFile(StringView absSrcPath, cpp::PPVisitedFiles* visitedFiles,
                         cpp::ParseSupervisor* visor);
} // namespace cpp
} // namespace ply

// FIXME: Just call this once per parsed file
String makeInlRelPath(StringView relPath) {
    auto splitPath = Path.split(relPath);
    return Path.join(splitPath.first, "codegen", Path.splitExt(splitPath.second).first + ".inl");
}

Subst insertDirectiveSubst(StringView view, const char* marker, String&& replacement) {
    Subst subst;
    subst.replacement = std::move(replacement);
    const char* insertPos = marker;
    while (view.bytes < insertPos) {
        char c = insertPos[-1];
        if (c == '\n') {
            break;
        } else if (!isWhite(c)) {
            const char* firstNonWhite = insertPos - 1;
            const char* lineStart = firstNonWhite;
            while (view.bytes < lineStart) {
                c = lineStart[-1];
                if (c == '\n')
                    break;
                lineStart--;
                if (!isWhite(c)) {
                    firstNonWhite = lineStart;
                }
            }
            subst.replacement =
                String::format("\n{}{}", subst.replacement,
                               StringView{lineStart, safeDemote<u32>(firstNonWhite - lineStart)});
            break;
        }
        insertPos--;
    }
    subst.start = safeDemote<u32>(insertPos - view.bytes);
    subst.numBytes = 0;
    return subst;
}

struct ReflectionHookError : cpp::BaseError {
    enum Type {
        // ply reflect enum
        Unknown,
        SwitchMayOnlyContainStructs,
        MissingReflectOffCommand,
        UnexpectedReflectOffCommand,
        CannotInjectCodeIntoMacro,
        DuplicateCommand,
        CommandCanOnlyBeUsedAtDeclarationScope,
        CommandCanOnlyBeUsedInClassOrStruct,
        CommandCanOnlyBeUsedInsideEnum,
        UnrecognizedCommand,
    };

    PLY_REFLECT()
    Type type = Unknown;
    cpp::LinearLocation linearLoc = -1;
    cpp::LinearLocation otherLoc = -1;
    // ply reflect off

    PLY_INLINE ReflectionHookError(Type type, cpp::LinearLocation linearLoc,
                                   cpp::LinearLocation otherLoc = -1)
        : type{type}, linearLoc{linearLoc}, otherLoc{otherLoc} {
    }
    virtual void writeMessage(OutStream& out,
                              const cpp::PPVisitedFiles* visitedFiles) const override;
};

PLY_DECLARE_TYPE_DESCRIPTOR(ReflectionHookError::Type)

void ReflectionHookError::writeMessage(OutStream& out,
                                       const cpp::PPVisitedFiles* visitedFiles) const {
    out.format("{}: error: ", expandFileLocation(visitedFiles, this->linearLoc).toString());
    switch (this->type) {
        case ReflectionHookError::SwitchMayOnlyContainStructs: {
            out << "a switch may only contain structs\n";
            break;
        }
        case ReflectionHookError::MissingReflectOffCommand: {
            out << "can't find matching // ply reflect off\n";
            break;
        }
        case ReflectionHookError::UnexpectedReflectOffCommand: {
            out << "unexpected // ply reflect off\n";
            break;
        }
        case ReflectionHookError::CannotInjectCodeIntoMacro: {
            out << "can't inject code inside macro\n";
            out.format("{}: note: for code injected by this command\n",
                         expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case ReflectionHookError::DuplicateCommand: {
            out << "duplicate command\n";
            out.format("{}: note: see previous command\n",
                         expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope: {
            out << "command can only be used at declaration scope\n";
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct: {
            out << "command can only be used inside a class or struct\n";
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedInsideEnum: {
            out << "command can only be used inside an enum\n";
            break;
        }
        case ReflectionHookError::UnrecognizedCommand: {
            out << "unrecognized command\n";
            break;
        }
        default: {
            out << "error message not implemented!\n";
            break;
        }
    }
}

struct ReflectionHooks : cpp::ParseSupervisor {
    struct State {
        ReflectedClass* clazz = nullptr;
        cpp::Token captureMembersToken;
        Owned<SwitchInfo> switch_ = nullptr;
        Owned<ReflectedEnum> enum_ = nullptr;
        cpp::Token keepReflectedEnumToken;
    };
    Array<State> stack;
    StringView filePath;
    ReflectionInfoAggregator* agg = nullptr;
    SingleFileReflectionInfo* sfri = nullptr;
    bool anyError = false;

    virtual void enter(AnyObject node) override {
        State& state = this->stack.append();

        if (node.is<cpp::grammar::DeclSpecifier::Enum_>()) {
            state.enum_ = new ReflectedEnum;
            state.enum_->cppInlPath = makeInlRelPath(this->filePath);
            state.enum_->namespacePrefix = this->getNamespacePrefix();
            state.enum_->enumName = this->getClassName("::", false);
            return;
        }

        if (this->stack.numItems() > 1) {
            State& parentState = this->stack.back(-2);

            if (parentState.switch_) {
                auto* record = node.safeCast<cpp::grammar::DeclSpecifier::Record>();
                if (!record || record->classKey.identifier == "union") {
                    this->parser->pp->errorHandler(
                        new ReflectionHookError{ReflectionHookError::SwitchMayOnlyContainStructs,
                                                parentState.switch_->macro.linearLoc});
                    return;
                }
                parentState.switch_->states.append(record->qid.toString());
                return;
            }
        }
    }

    virtual void exit(AnyObject node) override {
        State& state = this->stack.back();
        if (state.captureMembersToken.isValid()) {
            this->parser->pp->errorHandler(
                new ReflectionHookError{ReflectionHookError::MissingReflectOffCommand,
                                        state.captureMembersToken.linearLoc});
        }

        if (state.keepReflectedEnumToken.isValid()) {
            this->agg->enums.append(std::move(state.enum_));
        }

        if (state.switch_) {
            auto* record = node.safeCast<cpp::grammar::DeclSpecifier::Record>();
            PLY_ASSERT(record);                                 // guaranteed by enter()
            PLY_ASSERT(record->classKey.identifier != "union"); // guaranteed by enter()
            if (record->closeCurly.isValid()) {
                cpp::PPVisitedFiles* vf = this->parser->pp->visitedFiles;
                auto iter = vf->locationMap.findLastLessThan(record->closeCurly.linearLoc + 1);
                const cpp::PPVisitedFiles::LocationMapTraits::Item& lmItem = iter.getItem();
                const cpp::PPVisitedFiles::IncludeChain& chain =
                    vf->includeChains[lmItem.includeChainIdx];
                if (chain.isMacroExpansion) {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::CannotInjectCodeIntoMacro,
                        record->closeCurly.linearLoc, state.switch_->macro.linearLoc});
                } else {
                    const cpp::PPVisitedFiles::SourceFile& srcFile =
                        vf->sourceFiles[chain.fileOrExpIdx];
                    String curAbsPath = Path.join(Workspace.path, this->filePath);
                    // FIXME: Improve this if we ever start following includes while collecting
                    // reflection info:
                    PLY_ASSERT(srcFile.fileLocationMap.path == curAbsPath);
                    const char* endCurly =
                        srcFile.contents.bytes +
                        (lmItem.offset + record->closeCurly.linearLoc - lmItem.linearLoc);
                    PLY_ASSERT(*endCurly == '}');
                    String genFileName = String::format("switch-{}.inl", this->getClassName("-"));
                    state.switch_->inlineInlPath =
                        Path.join(Path.split(this->filePath).first, "codegen", genFileName);
                    this->sfri->substsInParsedFile.append(insertDirectiveSubst(
                        srcFile.contents, endCurly,
                        String::format("#include \"codegen/{}\" //@@ply\n", genFileName)));

                    // Add section(s) to .cpp .inl
                    this->sfri->switches.append(state.switch_);
                    this->agg->switches.append(std::move(state.switch_));
                }
            }
        }
        stack.pop();
    }

    virtual void onGotDeclaration(const cpp::grammar::Declaration& decl) override {
        State& state = this->stack.back();
        if (state.captureMembersToken.isValid()) {
            if (auto simple = decl.simple()) {
                if (!simple->initDeclarators.isEmpty()) {
                    const cpp::grammar::InitDeclaratorWithComma& initDecl =
                        simple->initDeclarators[0];
                    if (!initDecl.dcor.isFunction()) {
                        state.clazz->members.append(initDecl.dcor.qid.toString());
                    }
                }
            }
        }
    }

    virtual void onGotEnumerator(const cpp::grammar::InitEnumeratorWithComma* initEnor) override {
        State& state = this->stack.back();
        PLY_ASSERT(state.enum_);
        state.enum_->enumerators.append(initEnor->identifier.identifier);
    }

    virtual void onGotInclude(StringView directive) override {
        const cpp::Preprocessor::StackItem& ppItem = this->parser->pp->stack.back();
        const char* ppItemStartUnit = (const char*) ppItem.in.cur_byte;
        PLY_ASSERT(directive.bytes >= ppItemStartUnit &&
                   directive.end() <= (const char*) ppItem.in.end_byte);
        if (!directive.rtrim([](char c) { return isWhite(c); }).endsWith("//@@ply"))
            return;

        // This will delete the line containing the #include:
        const char* startOfLine = directive.bytes;
        while ((startOfLine > ppItemStartUnit) && (startOfLine[-1] != '\n')) {
            startOfLine--;
        }
        Subst& subst = this->sfri->substsInParsedFile.append();
        subst.start = safeDemote<u32>(startOfLine - ppItemStartUnit);
        subst.numBytes = safeDemote<u32>((directive.bytes + directive.numBytes) - startOfLine);
    }

    void beginCapture(const cpp::Token& token) {
        State& state = this->stack.back();
        if (state.clazz) {
            // Already have a PLY_REFLECT macro
            this->parser->pp->errorHandler(
                new ReflectionHookError{ReflectionHookError::DuplicateCommand, token.linearLoc,
                                        state.captureMembersToken.linearLoc});
            return;
        }
        ReflectedClass* clazz = new ReflectedClass;
        clazz->cppInlPath = makeInlRelPath(this->filePath);
        clazz->name = this->getClassName();
        this->agg->classes.append(clazz);
        state.clazz = clazz;
        state.captureMembersToken = token;
    }

    virtual void gotMacroOrComment(cpp::Token token) override {
        if (token.type == cpp::Token::Macro) {
            if (token.identifier == "PLY_STATE_REFLECT" || token.identifier == "PLY_REFLECT") {
                if (!this->parser->atDeclarationScope) {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                        token.linearLoc});
                    return;
                }
                auto* record =
                    this->scopeStack.back().safeCast<cpp::grammar::DeclSpecifier::Record>();
                if (!record || record->classKey.identifier == "union") {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct, token.linearLoc});
                    return;
                }
                this->beginCapture(token);
            }
        } else if (token.type == cpp::Token::LineComment) {
            ViewInStream commentReader{token.identifier};
            PLY_ASSERT(commentReader.view_readable().startsWith("//"));
            commentReader.cur_byte += 2;
            commentReader.parse<fmt::Whitespace>();
            if (commentReader.view_readable().startsWith("%%")) {
                commentReader.cur_byte += 2;
                commentReader.parse<fmt::Whitespace>();
                StringView cmd = commentReader.readView<fmt::Identifier>();
                if (cmd == "end") {
                    if (this->parser->atDeclarationScope) {
                        State& state = this->stack.back();
                        if (!state.captureMembersToken.isValid()) {
                            // this->parser->error(); // Not capturing
                            return;
                        }
                        state.captureMembersToken = {};
                    }
                }
            } else if (commentReader.view_readable().startsWith("ply ")) {
                String fixed = String::format(
                    "// {}\n",
                    StringView{" "}.join(commentReader.view_readable()
                                                        .rtrim([](char c) { return isWhite(c); })
                                                        .splitByte(' ')));
                if (fixed == "// ply make switch\n" || fixed == "// ply make reflected switch\n") {
                    // This is a switch
                    if (!this->parser->atDeclarationScope) {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                            token.linearLoc});
                        return;
                    }
                    auto* record =
                        this->scopeStack.back().safeCast<cpp::grammar::DeclSpecifier::Record>();
                    if (!record || record->classKey.identifier == "union") {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct,
                            token.linearLoc});
                        return;
                    }
                    State& state = this->stack.back();
                    if (state.switch_) {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::DuplicateCommand, token.linearLoc,
                            state.switch_->macro.linearLoc});
                        return;
                    }
                    SwitchInfo* switch_ = new SwitchInfo;
                    switch_->macro = token;
                    switch_->cppInlPath = makeInlRelPath(this->filePath);
                    switch_->name = this->getClassName();
                    state.switch_ = switch_;
                    if (fixed == "// ply make reflected switch\n") {
                        switch_->isReflected = true;
                    }
                } else if (fixed == "// ply reflect off\n") {
                    if (!this->parser->atDeclarationScope) {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                            token.linearLoc});
                        return;
                    }
                    State& state = this->stack.back();
                    if (!state.captureMembersToken.isValid()) {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::UnexpectedReflectOffCommand, token.linearLoc});
                        return;
                    }
                    state.captureMembersToken = {};
                } else if (fixed == "// ply reflect enum\n") {
                    auto* enum_ =
                        this->scopeStack.back().safeCast<cpp::grammar::DeclSpecifier::Enum_>();
                    if (!enum_) {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedInsideEnum, token.linearLoc});
                        return;
                    }
                    State& state = this->stack.back();
                    if (state.keepReflectedEnumToken.isValid()) {
                        this->parser->pp->errorHandler(new ReflectionHookError{
                            ReflectionHookError::DuplicateCommand, token.linearLoc,
                            state.keepReflectedEnumToken.linearLoc});
                        return;
                    }
                    state.keepReflectedEnumToken = token;
                } else {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::UnrecognizedCommand, token.linearLoc});
                    return;
                }
            }
        }
    }

    virtual bool handleError(Owned<cpp::BaseError>&& err) override {
        this->anyError = true;
        OutStream out = Console.error();
        err->writeMessage(out, this->parser->pp->visitedFiles);
        return true;
    }
};

Tuple<SingleFileReflectionInfo, bool> extractReflection(ReflectionInfoAggregator* agg,
                                                        StringView relPath) {
    SingleFileReflectionInfo sfri;
    ReflectionHooks visor;
    visor.filePath = relPath;
    visor.agg = agg;
    visor.sfri = &sfri;

    cpp::PPVisitedFiles visitedFiles;
    parsePlywoodSrcFile(Path.join(Workspace.path, relPath), &visitedFiles, &visor);

    return {std::move(sfri), !visor.anyError};
}

//                   ▄▄
//   ▄▄▄▄  ▄▄▄▄   ▄▄▄██  ▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ██    ██  ██ ██  ██ ██▄▄██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄▄ ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄▄  ▀█▄▄██ ▀█▄▄▄  ██  ██
//                              ▄▄▄█▀

struct CodeGenerator {
    virtual ~CodeGenerator() {
    }
    virtual void write(OutStream& out) = 0;
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
    String absInlPath = Path.join(Workspace.path, switch_->inlineInlPath);
    FSResult result = FileSystem.makeDirsAndSaveTextIfDifferent(
        absInlPath, getSwitchInl(switch_), tff);
    OutStream stdOut = Console.out();
    if (result == FSResult::OK) {
        stdOut.format("Wrote {}\n", absInlPath);
    } else if (result != FSResult::Unchanged) {
        stdOut.format("Error writing {}\n", absInlPath);
    }
}

String performSubsts(StringView absPath, ArrayView<Subst> substs) {
    String src = FileSystem.loadTextAutodetect(absPath);
    if (FileSystem.lastResult() != FSResult::OK)
        return {};

    MemOutStream mout;
    u32 prevEndPos = 0;
    for (const Subst& subst : substs) {
        PLY_ASSERT(subst.start >= prevEndPos);
        u32 endPos = subst.start + subst.numBytes;
        PLY_ASSERT(endPos < src.numBytes);
        mout << StringView{src.bytes + prevEndPos, subst.start - prevEndPos};
        mout << subst.replacement;
        prevEndPos = endPos;
    }
    mout << StringView{src.bytes + prevEndPos, src.numBytes - prevEndPos};
    return mout.moveToString();
}

void performSubstsAndSave(StringView absPath, ArrayView<Subst> substs, const TextFormat& tff) {
    // FIXME: Don't reload the file here!!!!!!!!!!
    // It may have changed, making the Substs invalid!!!!!!!!
    String srcWithSubst = performSubsts(absPath, substs);
    if (FileSystem.lastResult() == FSResult::OK) {
        FSResult result =
            FileSystem.makeDirsAndSaveTextIfDifferent(absPath, srcWithSubst, tff);
        OutStream stdOut = Console.out();
        if (result == FSResult::OK) {
            stdOut.format("Wrote {}\n", absPath);
        } else if (result != FSResult::Unchanged) {
            stdOut.format("Error writing {}\n", absPath);
        }
    }
}

void generateAllCppInls(ReflectionInfoAggregator* agg, const TextFormat& tff) {
    struct CodeGenerator {
        virtual ~CodeGenerator() {
        }
        virtual void write(OutStream& out) = 0;
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

    for (ReflectedClass* clazz : agg->classes) {
        struct StructGenerator : CodeGenerator {
            ReflectedClass* clazz;
            virtual void write(OutStream& out) override {
                out.format("PLY_STRUCT_BEGIN({})\n", this->clazz->name);
                for (StringView member : this->clazz->members) {
                    out.format("PLY_STRUCT_MEMBER({})\n", member);
                }
                out << "PLY_STRUCT_END()\n\n";
            }
            StructGenerator(ReflectedClass* clazz) : clazz{clazz} {
            }
        };
        auto cursor = fileToGeneratorList.insertOrFind(clazz->cppInlPath);
        cursor->sources.append(new StructGenerator{clazz});
    }

    for (ReflectedEnum* enum_ : agg->enums) {
        struct EnumGenerator : CodeGenerator {
            ReflectedEnum* enum_;
            virtual void write(OutStream& out) override {
                out.format("PLY_ENUM_BEGIN({}, {})\n", this->enum_->namespacePrefix,
                             this->enum_->enumName);
                for (u32 i = 0; i < this->enum_->enumerators.numItems(); i++) {
                    StringView enumerator = this->enum_->enumerators[i];
                    if ((i != this->enum_->enumerators.numItems() - 1) || (enumerator != "Count")) {
                        out.format("PLY_ENUM_IDENTIFIER({})\n", enumerator);
                    }
                }
                out.format("PLY_ENUM_END()\n\n");
            }
            EnumGenerator(ReflectedEnum* enum_) : enum_{enum_} {
            }
        };
        auto cursor = fileToGeneratorList.insertOrFind(enum_->cppInlPath);
        cursor->sources.append(new EnumGenerator{enum_});
    }

    for (SwitchInfo* switch_ : agg->switches) {
        struct SwitchGenerator : CodeGenerator {
            SwitchInfo* switch_;
            virtual void write(OutStream& out) override {
                out.format("SWITCH_TABLE_BEGIN({})\n", this->switch_->name);
                for (StringView state : this->switch_->states) {
                    out.format("SWITCH_TABLE_STATE({}, {})\n", this->switch_->name, state);
                }
                out.format("SWITCH_TABLE_END({})\n\n", this->switch_->name);

                if (this->switch_->isReflected) {
                    out.format("PLY_SWITCH_BEGIN({})\n", this->switch_->name);
                    for (StringView state : this->switch_->states) {
                        out.format("PLY_SWITCH_MEMBER({})\n", state);
                    }
                    out.format("PLY_SWITCH_END()\n\n");
                }
            }
            SwitchGenerator(SwitchInfo* switch_) : switch_{switch_} {
            }
        };
        auto cursor = fileToGeneratorList.insertOrFind(switch_->cppInlPath);
        cursor->sources.append(new SwitchGenerator{switch_});
    }

    for (const Traits::Item& item : fileToGeneratorList) {
        PLY_ASSERT(item.cppInlPath.endsWith(".inl"));
        String absPath = Path.join(Workspace.path, item.cppInlPath);

        MemOutStream mout;
        for (CodeGenerator* generator : item.sources) {
            generator->write(mout);
        }
        FSResult result =
            FileSystem.makeDirsAndSaveTextIfDifferent(absPath, mout.moveToString(), tff);
        OutStream stdOut = Console.out();
        if (result == FSResult::OK) {
            stdOut.format("Wrote {}\n", absPath);
        } else if (result != FSResult::Unchanged) {
            stdOut.format("Error writing {}\n", absPath);
        }
    }
}

void do_codegen() {
    ReflectionInfoAggregator agg;

    u32 fileNum = 0;
    for (const FileInfo& entry : FileSystem.listDir(Workspace.path, 0)) {
        if (!entry.isDir)
            continue;
        if (entry.name.startsWith("."))
            continue;
        if (entry.name == "data")
            continue;

        for (WalkTriple& triple :
             FileSystem.walk(Path.join(Workspace.path, entry.name))) {
            // Sort child directories and filenames so that files are visited in a deterministic
            // order:
            sort(triple.dirNames);
            sort(triple.files, [](const FileInfo& a, const FileInfo& b) {
                return a.name < b.name;
            });

            if (find(triple.files,
                     [](const auto& fileInfo) { return fileInfo.name == "nocodegen"; }) >= 0) {
                triple.dirNames.clear();
                continue;
            }

            for (const FileInfo& file : triple.files) {
                if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                    // FIXME: Eliminate exclusions
                    for (StringView exclude : {
                             "Sort.h",
                             "Func.h",
                             "DirectoryWatcher_Mac.h",
                             "DirectoryWatcher_Win32.h",
                             "Heap.cpp",
                             "Pool.h",
                         }) {
                        if (file.name == exclude)
                            goto skipIt;
                    }
                    {
                        fileNum++;

                        Tuple<SingleFileReflectionInfo, bool> sfri =
                            extractReflection(&agg, Path.join(triple.dirPath, file.name));
                        if (sfri.second) {
                            for (SwitchInfo* switch_ : sfri.first.switches) {
                                writeSwitchInl(switch_, Workspace.getSourceTextFormat());
                            }
                            performSubstsAndSave(Path.join(triple.dirPath, file.name),
                                                 sfri.first.substsInParsedFile,
                                                 Workspace.getSourceTextFormat());
                        }
                    }
                skipIt:;
                }
            }
            for (StringView exclude : {"Shell_iOS", "opengl-support"}) {
                s32 i = find(triple.dirNames, exclude);
                if (i >= 0) {
                    triple.dirNames.erase(i);
                }
            }
        }
    }

    generateAllCppInls(&agg, Workspace.getSourceTextFormat());
}

#include "codegen/codegen.inl" //%%
