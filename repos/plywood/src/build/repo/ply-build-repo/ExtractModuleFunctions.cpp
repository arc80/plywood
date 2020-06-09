/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/ExtractModuleFunctions.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {
void parsePlywoodSrcFile(StringView absSrcPath, cpp::PPVisitedFiles* visitedFiles,
                         ParseSupervisor* visor);
} // namespace cpp

namespace build {

struct RepoRegError : cpp::BaseError {
    enum Type {
        // ply reflect enum
        Unknown,
        AlreadyInsideCommand,
        ExpectedModuleName,
        MustBeAtFileScope,
        ExpectedExternName,
        ExpectedProviderKeyword,
        ExpectedProviderName,
        UnrecognizedCommand,
        CouldNotApplyCommand,
        ExpectedEqualSign,
        ExtraneousText,
        OldCommandFormat,
    };
    PLY_REFLECT_ENUM(friend, Type)

    PLY_REFLECT()
    Type type = Unknown;
    cpp::LinearLocation linearLoc = -1;
    cpp::LinearLocation otherLoc = -1;
    // ply reflect off

    PLY_INLINE RepoRegError(Type type, cpp::LinearLocation linearLoc,
                            cpp::LinearLocation otherLoc = -1)
        : type{type}, linearLoc{linearLoc}, otherLoc{otherLoc} {
    }
    virtual void writeMessage(StringWriter* sw,
                              const cpp::PPVisitedFiles* visitedFiles) const override;
};
PLY_REFLECT_ENUM(, RepoRegError::Type)

void RepoRegError::writeMessage(StringWriter* sw, const cpp::PPVisitedFiles* visitedFiles) const {
    sw->format("{}: error: ", cpp::expandFileLocation(visitedFiles, this->linearLoc).toString());
    switch (this->type) {
        case RepoRegError::AlreadyInsideCommand: {
            *sw << "already inside command\n";
            sw->format("{}: note: previous command started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case RepoRegError::ExpectedModuleName: {
            *sw << "expected module name in quotes\n";
            break;
        }
        case RepoRegError::MustBeAtFileScope: {
            *sw << "command can only be used at file scope\n";
            break;
        }
        case RepoRegError::ExpectedExternName: {
            *sw << "expected extern name in quotes\n";
            break;
        }
        case RepoRegError::ExpectedProviderKeyword: {
            *sw << "expected keyword 'provider'\n";
            break;
        }
        case RepoRegError::ExpectedProviderName: {
            *sw << "expected provider name in quotes\n";
            break;
        }
        case RepoRegError::UnrecognizedCommand: {
            *sw << "unrecognized command\n";
            break;
        }
        case RepoRegError::CouldNotApplyCommand: {
            *sw << "command can't be applied to the declaration that follows it\n";
            break;
        }
        case RepoRegError::ExpectedEqualSign: {
            *sw << "expected '='\n";
            break;
        }
        case RepoRegError::ExtraneousText: {
            *sw << "unexpected text after command\n";
            break;
        }
        case RepoRegError::OldCommandFormat: {
            *sw << "please update to the new command format: // [ply ...]\n";
            break;
        }
        default: {
            *sw << "error message not implemented!\n";
            break;
        }
    }
}

struct InstantiatorHooks : cpp::ParseSupervisor {
    struct Command {
        struct Type {
            // ply make switch
            struct Module {
                String moduleName;
            };
            struct External {
                String externName;
                String providerName;
            };
#include "codegen/switch-ply-build-InstantiatorHooks-Command-Type.inl" //@@ply
        };

        cpp::Token macro;
        Type type;
    };

    Owned<Command> cmd;
    ModuleDefinitionFile* modDefFile = nullptr;
    bool anyError = false;

    static PLY_INLINE cpp::LinearLocation getLinearLocation(const cpp::Token& token,
                                                            const u8* curByte) {
        PLY_ASSERT((const char*) curByte >= token.identifier.bytes);
        PLY_ASSERT((const char*) curByte <= token.identifier.end());
        return token.linearLoc + ((const char*) curByte - token.identifier.bytes);
    }

    virtual void gotMacroOrComment(cpp::Token token) override {
        if (this->parser->atDeclarationScope) {
            if (token.type == cpp::Token::LineComment) {
                PLY_ASSERT(token.identifier.startsWith("//"));
                StringView commentText = token.identifier.subStr(2).trim(isWhite);
                if (commentText.startsWith("[") && commentText.endsWith("]")) {
                    // New command format: [ply ...]
                    StringViewReader commentReader{commentText.subStr(1, commentText.numBytes - 2)};
                    StringView first = commentReader.readView<fmt::Identifier>();
                    if (first != "ply")
                        return;

                    if (this->cmd) {
                        this->parser->pp->errorHandler.call(
                            new RepoRegError{RepoRegError::AlreadyInsideCommand,
                                             getLinearLocation(token, (const u8*) first.bytes),
                                             this->cmd->macro.linearLoc});
                        this->cmd = nullptr;
                    }

                    commentReader.parse<fmt::Whitespace>();
                    StringView second = commentReader.readView<fmt::Identifier>();
                    if (second == "module") {
                        if (this->scopeStack.numItems() != 1) {
                            this->parser->pp->errorHandler.call(new RepoRegError{
                                RepoRegError::MustBeAtFileScope,
                                getLinearLocation(token, (const u8*) second.bytes)});
                            return;
                        }
                        commentReader.parse<fmt::Whitespace>();
                        if (!commentReader.viewAvailable().startsWith("=")) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExpectedEqualSign,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }
                        commentReader.advanceByte();
                        commentReader.parse<fmt::Whitespace>();
                        String moduleName = commentReader.parse<fmt::QuotedString>();
                        if (moduleName.isEmpty()) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExpectedModuleName,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }
                        commentReader.parse<fmt::Whitespace>();
                        if (commentReader.numBytesAvailable() > 0) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExtraneousText,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }

                        // Handle [ply module="<moduleName>"]
                        this->cmd = new Command;
                        this->cmd->macro = token;
                        auto moduleCmd = this->cmd->type.module().switchTo();
                        moduleCmd->moduleName = moduleName;
                    } else if (second == "extern") {
                        if (this->scopeStack.numItems() != 1) {
                            this->parser->pp->errorHandler.call(new RepoRegError{
                                RepoRegError::MustBeAtFileScope,
                                getLinearLocation(token, (const u8*) second.bytes)});
                            return;
                        }
                        commentReader.parse<fmt::Whitespace>();
                        if (!commentReader.viewAvailable().startsWith("=")) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExpectedEqualSign,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }
                        commentReader.advanceByte();
                        commentReader.parse<fmt::Whitespace>();
                        String externName = commentReader.parse<fmt::QuotedString>();
                        if (externName.isEmpty()) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExpectedExternName,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }
                        commentReader.parse<fmt::Whitespace>();
                        StringView third = commentReader.readView<fmt::Identifier>();
                        if (third != "provider") {
                            this->parser->pp->errorHandler.call(new RepoRegError{
                                RepoRegError::ExpectedProviderKeyword,
                                getLinearLocation(token, (const u8*) third.bytes)});
                            return;
                        }
                        commentReader.parse<fmt::Whitespace>();
                        if (!commentReader.viewAvailable().startsWith("=")) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExpectedEqualSign,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }
                        commentReader.advanceByte();
                        commentReader.parse<fmt::Whitespace>();
                        String providerName = commentReader.parse<fmt::QuotedString>();
                        if (providerName.isEmpty()) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExpectedProviderName,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }
                        commentReader.parse<fmt::Whitespace>();
                        if (commentReader.numBytesAvailable() > 0) {
                            this->parser->pp->errorHandler.call(
                                new RepoRegError{RepoRegError::ExtraneousText,
                                                 getLinearLocation(token, commentReader.curByte)});
                            return;
                        }

                        // Handle [ply extern="<externName>" provider="<providerName>"]
                        this->cmd = new Command;
                        this->cmd->macro = token;
                        auto externCmd = this->cmd->type.external().switchTo();
                        externCmd->externName = externName;
                        externCmd->providerName = providerName;
                    } else {
                        this->parser->pp->errorHandler.call(
                            new RepoRegError{RepoRegError::UnrecognizedCommand,
                                             getLinearLocation(token, (const u8*) second.bytes)});
                    }
                    return;
                }

                // FIXME: Delete this later
                StringViewReader commentReader{commentText};
                StringView first = commentReader.readView<fmt::NonWhitespace>();
                commentReader.parse<fmt::Whitespace>();
                if (first == "ply") {
                    this->parser->pp->errorHandler.call(
                        new RepoRegError{RepoRegError::OldCommandFormat, token.linearLoc});
                }
            }
        }
    }

    virtual void onGotDeclaration(const cpp::grammar::Declaration& decl) override {
        if (this->cmd) {
            bool appliedCommand = false;
            if (auto simple = decl.simple()) {
                if (simple->initDeclarators.numItems() == 1) {
                    const cpp::grammar::InitDeclaratorWithComma& initDcor =
                        simple->initDeclarators[0];
                    if (initDcor.dcor.isFunction()) {
                        if (auto moduleCmd = this->cmd->type.module()) {
                            ModuleDefinitionFile::ModuleFunc& moduleFunc =
                                this->modDefFile->moduleFuncs.append();
                            moduleFunc.funcName = initDcor.dcor.qid.toString();
                            moduleFunc.moduleName = moduleCmd->moduleName;
                            this->cmd = nullptr;
                        } else if (auto externCmd = this->cmd->type.external()) {
                            ModuleDefinitionFile::ExternProviderFunc& externProviderFunc =
                                this->modDefFile->externProviderFuncs.append();
                            externProviderFunc.externName = externCmd->externName;
                            externProviderFunc.providerName = externCmd->providerName;
                            externProviderFunc.funcName = initDcor.dcor.qid.toString();
                            this->cmd = nullptr;
                        } else {
                            PLY_ASSERT(0);
                        }
                        appliedCommand = true;
                    }
                }
            }
            if (!appliedCommand) {
                this->parser->pp->errorHandler.call(new RepoRegError{
                    RepoRegError::CouldNotApplyCommand, this->cmd->macro.linearLoc});
                this->cmd = nullptr;
            }
        }
    }

    virtual bool handleError(Owned<cpp::BaseError>&& err) override {
        this->anyError = true;
        StringWriter sw;
        err->writeMessage(&sw, this->parser->pp->visitedFiles);
        StdErr::createStringWriter() << sw.moveToString();
        return true;
    }
};

bool extractInstantiatorFunctions(ModuleDefinitionFile* modDefFile) {
    InstantiatorHooks visor;
    visor.modDefFile = modDefFile;

    cpp::PPVisitedFiles visitedFiles;
    parsePlywoodSrcFile(modDefFile->absPath, &visitedFiles, &visor);

    return !visor.anyError;
}

} // namespace build
} // namespace ply

#include "codegen/ExtractModuleFunctions.inl" //%%
