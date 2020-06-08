/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/ExtractInstantiatorFunctions.h>
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
        ExpectedTargetName,
        MustBeAtFileScope,
        ExpectedExternName,
        ExpectedProviderName,
        UnrecognizedCommand,
        CouldNotApplyCommand,
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
        case RepoRegError::ExpectedTargetName: {
            *sw << "expected target name\n";
            break;
        }
        case RepoRegError::MustBeAtFileScope: {
            *sw << "command can only be used at file scope\n";
            break;
        }
        case RepoRegError::ExpectedExternName: {
            *sw << "expected extern name\n";
            break;
        }
        case RepoRegError::ExpectedProviderName: {
            *sw << "expected provider name\n";
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
            struct Target {
                StringView targetName;
            };
            struct External {
                StringView externName;
                StringView providerName;
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
                StringViewReader commentReader{token.identifier};
                PLY_ASSERT(commentReader.viewAvailable().startsWith("//"));
                commentReader.advanceByte(2);
                commentReader.parse<fmt::Whitespace>();
                StringView first = commentReader.readView<fmt::NonWhitespace>();
                commentReader.parse<fmt::Whitespace>();
                if (first != "ply")
                    return;
                if (this->cmd) {
                    this->parser->pp->errorHandler.call(
                        new RepoRegError{RepoRegError::AlreadyInsideCommand,
                                         getLinearLocation(token, (const u8*) first.bytes),
                                         this->cmd->macro.linearLoc});
                    this->cmd = nullptr;
                }

                StringView second = commentReader.readView<fmt::NonWhitespace>();
                commentReader.parse<fmt::Whitespace>();
                if (second == "instantiate") {
                    if (this->scopeStack.numItems() != 1) {
                        this->parser->pp->errorHandler.call(
                            new RepoRegError{RepoRegError::MustBeAtFileScope,
                                             getLinearLocation(token, (const u8*) second.bytes)});
                        return;
                    }
                    StringView name = commentReader.readView<fmt::NonWhitespace>();
                    if (name.isEmpty()) {
                        this->parser->pp->errorHandler.call(
                            new RepoRegError{RepoRegError::ExpectedTargetName,
                                             getLinearLocation(token, commentReader.curByte)});
                        return;
                    }
                    commentReader.parse<fmt::Whitespace>();
                    this->cmd = new Command;
                    this->cmd->macro = token;
                    auto targetCmd = this->cmd->type.target().switchTo();
                    targetCmd->targetName = name;
                } else if (second == "extern") {
                    commentReader.parse<fmt::Whitespace>();
                    StringView providerName = commentReader.readView<fmt::NonWhitespace>();
                    if (providerName.isEmpty()) {
                        this->parser->pp->errorHandler.call(
                            new RepoRegError{RepoRegError::ExpectedProviderName,
                                             getLinearLocation(token, commentReader.curByte)});
                        return;
                    }
                    commentReader.parse<fmt::Whitespace>();
                    this->cmd = new Command;
                    this->cmd->macro = token;
                    auto externCmd = this->cmd->type.external().switchTo();
                    externCmd->providerName = providerName;
                } else {
                    this->parser->pp->errorHandler.call(
                        new RepoRegError{RepoRegError::UnrecognizedCommand,
                                         getLinearLocation(token, (const u8*) second.bytes)});
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
                        if (auto targetCmd = this->cmd->type.target()) {
                            ModuleDefinitionFile::TargetFunc& targetFunc =
                                this->modDefFile->targetFuncs.append();
                            targetFunc.funcName = initDcor.dcor.qid.toString();
                            targetFunc.targetName = targetCmd->targetName;
                            this->cmd = nullptr;
                        } else if (auto externCmd = this->cmd->type.external()) {
                            ModuleDefinitionFile::ExternProviderFunc& externProviderFunc =
                                this->modDefFile->externProviderFuncs.append();
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

#include "codegen/ExtractInstantiatorFunctions.inl" //%%
