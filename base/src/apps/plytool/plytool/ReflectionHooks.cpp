/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ReflectionHooks.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

void parsePlywoodSrcFile(StringView absSrcPath, cpp::PPVisitedFiles* visitedFiles,
                         ParseSupervisor* visor);

// FIXME: Just call this once per parsed file
String makeInlRelPath(StringView relPath) {
    auto splitPath = NativePath::split(relPath);
    return NativePath::join(splitPath.first, "codegen",
                            NativePath::splitExt(splitPath.second).first + ".inl");
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

struct ReflectionHookError : BaseError {
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
    LinearLocation linearLoc = -1;
    LinearLocation otherLoc = -1;
    // ply reflect off

    PLY_INLINE ReflectionHookError(Type type, LinearLocation linearLoc,
                                   LinearLocation otherLoc = -1)
        : type{type}, linearLoc{linearLoc}, otherLoc{otherLoc} {
    }
    virtual void writeMessage(OutStream* outs, const PPVisitedFiles* visitedFiles) const override;
};

} // namespace cpp
PLY_DECLARE_TYPE_DESCRIPTOR(cpp::ReflectionHookError::Type)
namespace cpp {

void ReflectionHookError::writeMessage(OutStream* outs, const PPVisitedFiles* visitedFiles) const {
    outs->format("{}: error: ", expandFileLocation(visitedFiles, this->linearLoc).toString());
    switch (this->type) {
        case ReflectionHookError::SwitchMayOnlyContainStructs: {
            *outs << "a switch may only contain structs\n";
            break;
        }
        case ReflectionHookError::MissingReflectOffCommand: {
            *outs << "can't find matching // ply reflect off\n";
            break;
        }
        case ReflectionHookError::UnexpectedReflectOffCommand: {
            *outs << "unexpected // ply reflect off\n";
            break;
        }
        case ReflectionHookError::CannotInjectCodeIntoMacro: {
            *outs << "can't inject code inside macro\n";
            outs->format("{}: note: for code injected by this command\n",
                         expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case ReflectionHookError::DuplicateCommand: {
            *outs << "duplicate command\n";
            outs->format("{}: note: see previous command\n",
                         expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope: {
            *outs << "command can only be used at declaration scope\n";
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct: {
            *outs << "command can only be used inside a class or struct\n";
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedInsideEnum: {
            *outs << "command can only be used inside an enum\n";
            break;
        }
        case ReflectionHookError::UnrecognizedCommand: {
            *outs << "unrecognized command\n";
            break;
        }
        default: {
            *outs << "error message not implemented!\n";
            break;
        }
    }
}

struct ReflectionHooks : ParseSupervisor {
    struct State {
        ReflectedClass* clazz = nullptr;
        Token captureMembersToken;
        Owned<SwitchInfo> switch_ = nullptr;
        Owned<ReflectedEnum> enum_ = nullptr;
        Token keepReflectedEnumToken;
    };
    Array<State> stack;
    StringView filePath;
    ReflectionInfoAggregator* agg = nullptr;
    SingleFileReflectionInfo* sfri = nullptr;
    bool anyError = false;

    virtual void enter(AnyObject node) override {
        State& state = this->stack.append();

        if (node.is<grammar::DeclSpecifier::Enum_>()) {
            state.enum_ = new ReflectedEnum;
            state.enum_->cppInlPath = makeInlRelPath(this->filePath);
            state.enum_->namespacePrefix = this->getNamespacePrefix();
            state.enum_->enumName = this->getClassName("::", false);
            return;
        }

        if (this->stack.numItems() > 1) {
            State& parentState = this->stack.back(-2);

            if (parentState.switch_) {
                auto* record = node.safeCast<grammar::DeclSpecifier::Record>();
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
            auto* record = node.safeCast<grammar::DeclSpecifier::Record>();
            PLY_ASSERT(record);                                 // guaranteed by enter()
            PLY_ASSERT(record->classKey.identifier != "union"); // guaranteed by enter()
            if (record->closeCurly.isValid()) {
                PPVisitedFiles* vf = this->parser->pp->visitedFiles;
                auto iter = vf->locationMap.findLastLessThan(record->closeCurly.linearLoc + 1);
                const PPVisitedFiles::LocationMapTraits::Item& lmItem = iter.getItem();
                const PPVisitedFiles::IncludeChain& chain =
                    vf->includeChains[lmItem.includeChainIdx];
                if (chain.isMacroExpansion) {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::CannotInjectCodeIntoMacro,
                        record->closeCurly.linearLoc, state.switch_->macro.linearLoc});
                } else {
                    const PPVisitedFiles::SourceFile& srcFile = vf->sourceFiles[chain.fileOrExpIdx];
                    String curAbsPath = NativePath::join(PLY_WORKSPACE_FOLDER, this->filePath);
                    // FIXME: Improve this if we ever start following includes while collecting
                    // reflection info:
                    PLY_ASSERT(srcFile.fileLocationMap.path == curAbsPath);
                    const char* endCurly =
                        srcFile.contents.bytes +
                        (lmItem.offset + record->closeCurly.linearLoc - lmItem.linearLoc);
                    PLY_ASSERT(*endCurly == '}');
                    String genFileName = String::format("switch-{}.inl", this->getClassName("-"));
                    state.switch_->inlineInlPath = NativePath::join(
                        NativePath::split(this->filePath).first, "codegen", genFileName);
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

    virtual void onGotDeclaration(const grammar::Declaration& decl) override {
        State& state = this->stack.back();
        if (state.captureMembersToken.isValid()) {
            if (auto simple = decl.simple()) {
                if (!simple->initDeclarators.isEmpty()) {
                    const grammar::InitDeclaratorWithComma& initDecl = simple->initDeclarators[0];
                    if (!initDecl.dcor.isFunction()) {
                        state.clazz->members.append(initDecl.dcor.qid.toString());
                    }
                }
            }
        }
    }

    virtual void onGotEnumerator(const grammar::InitEnumeratorWithComma* initEnor) override {
        State& state = this->stack.back();
        PLY_ASSERT(state.enum_);
        state.enum_->enumerators.append(initEnor->identifier.identifier);
    }

    virtual void onGotInclude(StringView directive) override {
        const Preprocessor::StackItem& ppItem = this->parser->pp->stack.back();
        const char* ppItemStartUnit = (const char*) ppItem.vins.getStartByte();
        PLY_ASSERT(directive.bytes >= ppItemStartUnit &&
                   directive.end() <= (const char*) ppItem.vins.endByte);
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

    void beginCapture(const Token& token) {
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

    virtual void gotMacroOrComment(Token token) override {
        if (token.type == Token::Macro) {
            if (token.identifier == "PLY_STATE_REFLECT" || token.identifier == "PLY_REFLECT") {
                if (!this->parser->atDeclarationScope) {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                        token.linearLoc});
                    return;
                }
                auto* record = this->scopeStack.back().safeCast<grammar::DeclSpecifier::Record>();
                if (!record || record->classKey.identifier == "union") {
                    this->parser->pp->errorHandler(new ReflectionHookError{
                        ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct, token.linearLoc});
                    return;
                }
                this->beginCapture(token);
            }
        } else if (token.type == Token::LineComment) {
            ViewInStream commentReader{token.identifier};
            PLY_ASSERT(commentReader.viewAvailable().startsWith("//"));
            commentReader.advanceByte(2);
            commentReader.parse<fmt::Whitespace>();
            if (commentReader.viewAvailable().startsWith("%%")) {
                commentReader.advanceByte(2);
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
            } else if (commentReader.viewAvailable().startsWith("ply ")) {
                String fixed = String::format(
                    "// {}\n", StringView{" "}.join(commentReader.viewAvailable()
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
                        this->scopeStack.back().safeCast<grammar::DeclSpecifier::Record>();
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
                    auto* enum_ = this->scopeStack.back().safeCast<grammar::DeclSpecifier::Enum_>();
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
        OutStream outs = StdErr::text();
        err->writeMessage(&outs, this->parser->pp->visitedFiles);
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
    parsePlywoodSrcFile(NativePath::join(PLY_WORKSPACE_FOLDER, relPath), &visitedFiles, &visor);

    return {std::move(sfri), !visor.anyError};
}

} // namespace cpp
} // namespace ply

#include "codegen/ReflectionHooks.inl" //%%
