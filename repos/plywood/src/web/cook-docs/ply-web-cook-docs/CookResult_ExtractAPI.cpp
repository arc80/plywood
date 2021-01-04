/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>
#include <plytool-client/PlyToolClient.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/ParseAPI.h>
#include <ply-cpp/ErrorFormatting.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace cpp {
void parsePlywoodSrcFile(StringView absSrcPath, cpp::PPVisitedFiles* visitedFiles,
                         ParseSupervisor* visor);
Array<sema::SingleDeclaration> semaFromParseTree(const grammar::Declaration::Simple& gSimple,
                                                 const cpp::PPVisitedFiles* visitedFiles);
sema::SingleDeclaration semaFromParam(const grammar::ParamDeclarationWithComma& gParam,
                                      const cpp::PPVisitedFiles* visitedFiles);
sema::QualifiedID semaFromQID(const grammar::QualifiedID& gQID,
                              const cpp::PPVisitedFiles* visitedFiles);
} // namespace cpp

namespace docs {

extern cook::CookJobType CookJobType_ExtractAPI;

// Fundamentally, the CookResult_ExtractAPI is the owner of all class/member SemaEntities
// namespace SemaEntities are owned indirectly via reference counting
struct CookResult_ExtractAPI : cook::CookResult {
    PLY_REFLECT();
    // ply reflect off
    Array<Reference<SemaEntity>> extractedClasses;
    Array<Reference<SemaEntity>> extractedAtNamespaceScope;
};

struct APIExtractor : cpp::ParseSupervisor {
    struct Error : cpp::BaseError {
        enum Type {
            // ply reflect enum
            Unknown = 0,
            AlreadyDefined,
            StrayMarkdown,
            BadIndentInDocumentationComment,
            NestedNamesNotSupported, // yet
            DirectivesMustBeAtStartOfDocumentationComment,
            EmptyDirective,
            BadDirective,
            DirectiveDoesNotTakeArguments,
            AlreadyInsideGroup,
            UnterminatedGroup,
            EndGroupOutsideGroup,
            GroupHasNoMarkdown,
            StrayMarkdownInGroup,
            EmptyDocumentationComment,
            DirectiveOnlyValidWithinClass,
            MarkdownOnlyValidWithinClass,
            DirectiveNotValidWithinClass,
            ExpectedClassName,
            UnexpectedAfterClassName,
            ClassNotFound,
            BeginParseTitleError,
            EndParseTitleError = BeginParseTitleError + (u32) ParseTitleError::NumErrors,
        };

        PLY_REFLECT();
        Type type = Unknown;
        String arg;
        cpp::LinearLocation linearLoc = -1;
        // ply reflect off

        PLY_INLINE Error(Type type, String arg, cpp::LinearLocation linearLoc)
            : type{type}, arg{arg}, linearLoc{linearLoc} {
        }
        virtual void writeMessage(StringWriter* sw,
                                  const cpp::PPVisitedFiles* visitedFiles) const override;
    };

    struct DocState {
        String markdown;
        cpp::LinearLocation markdownLoc = -1;
        cpp::LinearLocation groupDirectiveLoc = -1;
        DocInfo::Entry* groupEntry = nullptr;
        s32 categoryIndex = -1;
        SemaEntity* addToClass = nullptr;
    };

    Array<Reference<SemaEntity>> semaScopeStack;
    CookResult_ExtractAPI* extractAPIResult = nullptr;
    Array<Owned<cpp::BaseError>>* errors = nullptr;
    DocState docState;

    PLY_NO_INLINE void error(Error::Type type, StringView arg, cpp::LinearLocation linearLoc) {
        this->parser->pp->errorHandler.call(new Error{type, arg, linearLoc});
    }

    struct ScopeInfo {
        s32 parentIdx = -1;
        s32 templateIdx = -1;
        SemaEntity* parentScope = nullptr;
    };

    ScopeInfo getScopeInfo(s32 parentIdx) {
        PLY_ASSERT(this->scopeStack.numItems() == this->semaScopeStack.numItems());
        // Get template & parent indices
        ScopeInfo scopeInfo;
        if (parentIdx >= 0) {
            scopeInfo.parentIdx = parentIdx;
            if (this->semaScopeStack[scopeInfo.parentIdx]) {
                while (this->scopeStack[scopeInfo.parentIdx]
                           .safeCast<cpp::grammar::Declaration::Template_>()) {
                    if (scopeInfo.templateIdx < 0) {
                        scopeInfo.templateIdx = scopeInfo.parentIdx;
                    }
                    scopeInfo.parentIdx--;
                }
                scopeInfo.parentScope = this->semaScopeStack[scopeInfo.parentIdx];
            }
        }
        return scopeInfo;
    }

    virtual void enter(TypedPtr node) override {
        if (node.safeCast<cpp::grammar::TranslationUnit>()) {
            PLY_ASSERT(this->semaScopeStack.numItems() == 1);
            PLY_ASSERT(this->semaScopeStack[0]);
            return;
        }

        bool checkDocState = true;
        PLY_ON_SCOPE_EXIT({
            if (checkDocState) {
                if (this->docState.groupDirectiveLoc >= 0) {
                    this->error(Error::UnterminatedGroup, {}, this->docState.groupDirectiveLoc);
                    this->docState.groupDirectiveLoc = -1;
                    this->docState.groupEntry = nullptr;
                }
                if (this->docState.markdownLoc >= 0) {
                    this->error(Error::StrayMarkdown, {}, this->docState.markdownLoc);
                    this->docState.markdown = {};
                    this->docState.markdownLoc = -1;
                }
                this->docState.categoryIndex = -1;
                this->docState.addToClass = nullptr;
            }
        });

        Reference<SemaEntity>& curStackEntry = this->semaScopeStack.append();
        ScopeInfo scopeInfo = this->getScopeInfo(this->semaScopeStack.numItems() - 2);
        if (!scopeInfo.parentScope)
            return; // Early out if no parent

        if (auto* record = node.safeCast<cpp::grammar::DeclSpecifier::Record>()) {
            if (record->qid.nestedName.numItems() > 0) {
                // It's a class/struct/union declaration with a nested name specifier
                return;
            }
            auto ident = record->qid.unqual.identifier();
            if (!ident) {
                // It's an unnamed class or union
                return;
            }
            for (auto iter = scopeInfo.parentScope->nameToChild.findFirstGreaterOrEqualTo(
                     ident->name.identifier);
                 iter.isValid() && iter.getItem()->name == ident->name.identifier; iter.next()) {
                this->error(Error::AlreadyDefined, ident->name.identifier,
                            record->qid.getFirstToken().linearLoc);
                // Create the classEnt anyway
            }
            Reference<SemaEntity> classEnt = new SemaEntity;
            classEnt->setParent(scopeInfo.parentScope);
            classEnt->type = SemaEntity::Class;
            classEnt->name = ident->name.identifier;
            classEnt->docInfo = new DocInfo;
            classEnt->docInfo->class_ = classEnt;
            if (this->docState.markdownLoc >= 0) {
                classEnt->docInfo->classMarkdownDesc = std::move(this->docState.markdown);
                this->docState.markdownLoc = -1;
            }
            for (const cpp::grammar::BaseSpecifierWithComma& base : record->baseSpecifierList) {
                cpp::sema::QualifiedID qid =
                    cpp::semaFromQID(base.baseQid, this->parser->pp->visitedFiles);
                if (!qid.isEmpty()) {
                    classEnt->baseClasses.append(std::move(qid));
                }
            }

            scopeInfo.parentScope->nameToChild.insert(classEnt);
            if (scopeInfo.parentScope->type == SemaEntity::Namespace) {
                // FIXME: Only save the classEnt if it has docstrings (?)
                this->extractAPIResult->extractedClasses.append(classEnt);
            } else {
                PLY_ASSERT(scopeInfo.parentScope->type == SemaEntity::Class);
                scopeInfo.parentScope->childSeq.append(classEnt);
            }
            curStackEntry = std::move(classEnt);
            return;
        } else if (auto* ns_ = node.safeCast<cpp::grammar::Declaration::Namespace_>()) {
            if (ns_->qid.nestedName.numItems() > 0) {
                this->error(Error::Unknown, {}, record->qid.getFirstToken().linearLoc);
                return;
            }
            auto ident = ns_->qid.unqual.identifier();
            if (!ident) {
                this->error(Error::Unknown, {}, record->qid.getFirstToken().linearLoc);
                return;
            }
            for (auto iter = scopeInfo.parentScope->nameToChild.findFirstGreaterOrEqualTo(
                     ident->name.identifier);
                 iter.isValid() && iter.getItem()->name == ident->name.identifier; iter.next()) {
                if (iter.getItem()->type == SemaEntity::Namespace) {
                    // re-entering the same namespace
                    curStackEntry = iter.getItem();
                    return;
                } else {
                    this->error(Error::AlreadyDefined, ident->name.identifier,
                                record->qid.getFirstToken().linearLoc);
                    // FIXME: Should create the namespace anyway?
                    // This matters in order to get incremental cooking working
                    return;
                }
            }
            // Add new namespace to outer scope
            Reference<SemaEntity> namespaceEnt = new SemaEntity;
            namespaceEnt->setParent(scopeInfo.parentScope);
            namespaceEnt->type = SemaEntity::Namespace;
            namespaceEnt->name = ident->name.identifier;
            scopeInfo.parentScope->nameToChild.insert(namespaceEnt);
            curStackEntry = std::move(namespaceEnt);
            return;
        } else if (auto* template_ = node.safeCast<cpp::grammar::Declaration::Template_>()) {
            checkDocState = false;
            Reference<SemaEntity> templateParams = new SemaEntity;
            templateParams->type = SemaEntity::TemplateParamList;
            for (const cpp::grammar::ParamDeclarationWithComma& gParam : template_->params.params) {
                Reference<SemaEntity> sParam = new SemaEntity;
                sParam->type = SemaEntity::TemplateParam;
                if (auto ident = gParam.dcor.qid.unqual.identifier()) {
                    sParam->name = ident->name.identifier;
                }
                sParam->singleDecl = cpp::semaFromParam(gParam, this->parser->pp->visitedFiles);
                templateParams->childSeq.append(sParam);
                if (sParam->name) {
                    templateParams->nameToChild.insert(sParam);
                }
            }
            curStackEntry = templateParams;
            return;
        } else if (node.is<cpp::grammar::Declaration::Simple>()) {
            // Will be handled in onGotDeclaration
            checkDocState = false;
            return;
        }
    }

    virtual void exit(TypedPtr node) override {
        if (node.safeCast<cpp::grammar::DeclSpecifier::Record>() ||
            node.safeCast<cpp::grammar::Declaration::Namespace_>()) {
            if (this->docState.markdownLoc >= 0) {
                this->error(Error::StrayMarkdown, {}, this->docState.markdownLoc);
                this->docState.markdown = {};
                this->docState.markdownLoc = -1;
            }
            if (this->docState.groupDirectiveLoc >= 0) {
                this->error(Error::UnterminatedGroup, {}, this->docState.groupDirectiveLoc);
                this->docState.groupDirectiveLoc = -1;
                this->docState.groupEntry = nullptr;
            }
            this->docState.categoryIndex = -1;
            this->docState.addToClass = nullptr;
        }
        this->semaScopeStack.pop();
    }

    void parseDocString(const cpp::Token& token) {
        ScopeInfo scopeInfo = this->getScopeInfo(this->scopeStack.numItems() - 1);

        // Figure out indent
        cpp::ExpandedFileLocation exp =
            cpp::expandFileLocation(this->parser->pp->visitedFiles, token.linearLoc);
        u32 indent = exp.fileLoc.columnNumber;
        StringWriter sw;
        PLY_ASSERT(token.identifier.startsWith("/*!"));
        PLY_ASSERT(token.identifier.endsWith("*/"));
        const char* curByte = token.identifier.bytes + 3;
        const char* endByte = token.identifier.end() - 2;
        bool skipIndentation = false;
        while (curByte < endByte) {
            char c = *curByte;
            if (!isWhite(c))
                break;
            curByte++;
            if (c == '\n') {
                skipIndentation = true;
                break;
            }
        }

        // Loop over each line
        bool gotNonGroupDirective = false;
        bool gotAnyMarkdown = false;
        bool logIndentationError = true;
        for (;;) {
            // Skip indentation on new lines
            u32 column = 1;
            if (skipIndentation) {
                for (;;) {
                    if (curByte >= endByte)
                        goto endOfComment;
                    char c = *curByte++;
                    if (c == '\n') {
                        if (sw.getSeekPos() > 0) {
                            // Add blank line to the markdown
                            sw << c;
                            column = 1;
                        }
                    } else if (c == ' ') {
                        column++;
                    } else if (c == '\t') {
                        u32 tabSize = 4; // FIXME: Make configurable
                        column += tabSize - (column % tabSize);
                    } else {
                        // Got non-whitespace
                        curByte--;
                        break;
                    }
                }
            } else {
                column = indent;
                skipIndentation = true;
            }

            // There is text on this line
            cpp::LinearLocation lineLoc = token.linearLoc + (curByte - token.identifier.bytes);
            if (logIndentationError && column < indent) {
                this->error(Error::BadIndentInDocumentationComment, {}, lineLoc);
                logIndentationError = false;
            }

            // Get remaining part of the line
            const char* endOfLine = curByte;
            while (endOfLine < endByte) {
                if (*endOfLine++ == '\n')
                    break;
            }
            StringView line = StringView::fromRange(curByte, endOfLine);
            curByte = endOfLine;

            // Check for directives
            if (line.startsWith("\\")) {
                // This is a directive
                if (sw.getSeekPos() > 0) {
                    // Finish reading markdown
                    this->docState.markdown = sw.moveToString();
                    sw = {};
                }
                StringViewReader dr{line.subStr(1)};
                StringView directive = dr.readView<fmt::Identifier>();
                dr.parse<fmt::Whitespace>();
                if (directive != "beginGroup") {
                    gotNonGroupDirective = true;
                }
                if (!directive) {
                    this->error(Error::EmptyDirective, {}, lineLoc);
                } else if (directive == "beginGroup") {
                    // \beginGroup directive
                    if (dr.viewAvailable().trim(isWhite)) {
                        this->error(Error::DirectiveDoesNotTakeArguments, directive, lineLoc);
                    }
                    if (this->docState.groupDirectiveLoc >= 0) {
                        this->error(Error::AlreadyInsideGroup, {}, lineLoc);
                        this->error(Error::UnterminatedGroup, {}, this->docState.groupDirectiveLoc);
                        this->docState.groupDirectiveLoc = -1;
                        this->docState.groupEntry = nullptr;
                    }
                    if ((scopeInfo.parentScope &&
                         scopeInfo.parentScope->type == SemaEntity::Class) ||
                        this->docState.addToClass) {
                        this->docState.groupDirectiveLoc = lineLoc;
                    } else {
                        this->error(Error::DirectiveOnlyValidWithinClass, directive, lineLoc);
                    }
                } else if (directive == "endGroup") {
                    // \endGroup directive
                    if (dr.viewAvailable().trim(isWhite)) {
                        this->error(Error::DirectiveDoesNotTakeArguments, directive, lineLoc);
                    }
                    if (this->docState.groupDirectiveLoc < 0) {
                        this->error(Error::EndGroupOutsideGroup, directive, lineLoc);
                    }
                    this->docState.groupDirectiveLoc = -1;
                    this->docState.groupEntry = nullptr;
                } else if (directive == "category") {
                    if ((scopeInfo.parentScope &&
                         scopeInfo.parentScope->type == SemaEntity::Class) ||
                        this->docState.addToClass) {
                        SemaEntity* forClass = this->docState.addToClass;
                        if (!this->docState.addToClass) {
                            forClass = scopeInfo.parentScope;
                        }
                        StringView categoryDesc = dr.viewAvailable().trim(isWhite);
                        s32 categoryIndex = find(
                            forClass->docInfo->categories.view(),
                            [&](const DocInfo::Category& cat) { return cat.desc == categoryDesc; });
                        if (categoryIndex < 0) {
                            categoryIndex = forClass->docInfo->categories.numItems();
                            forClass->docInfo->categories.append(categoryDesc);
                        }
                        this->docState.categoryIndex = categoryIndex;
                    } else {
                        this->error(Error::DirectiveOnlyValidWithinClass, directive, lineLoc);
                    }
                } else if (directive == "addToClass") {
                    // There are serious limitations on \addToClass right now.
                    if (scopeInfo.parentScope && scopeInfo.parentScope->type == SemaEntity::Class) {
                        this->error(Error::DirectiveNotValidWithinClass, directive, lineLoc);
                    } else {
                        StringView className = dr.readView<fmt::Identifier>();
                        if (className.isEmpty()) {
                            this->error(Error::ExpectedClassName, directive, lineLoc);
                        } else {
                            if (dr.viewAvailable().trim(isWhite)) {
                                this->error(Error::UnexpectedAfterClassName, directive, lineLoc);
                            }
                            s32 i =
                                find(this->extractAPIResult->extractedClasses.view(),
                                     [&](const SemaEntity* ent) { return ent->name == className; });
                            if (i >= 0) {
                                this->docState.addToClass =
                                    this->extractAPIResult->extractedClasses[i];
                            } else {
                                this->error(Error::ClassNotFound, className, lineLoc);
                            }
                        }
                    }
                } else {
                    this->error(Error::BadDirective, directive, lineLoc);
                }
            } else {
                // Not a directive; treat as markdown
                gotAnyMarkdown = true;
                if (sw.getSeekPos() == 0) {
                    if (this->docState.groupEntry) {
                        this->error(Error::StrayMarkdownInGroup, {}, lineLoc);
                    } else {
                        if (this->docState.markdownLoc >= 0) {
                            this->error(Error::StrayMarkdown, {}, this->docState.markdownLoc);
                        }
                        this->docState.markdown.clear();
                        this->docState.markdownLoc = lineLoc;
                    }
                }
                for (u32 i = indent; i < column; i++) {
                    sw << ' ';
                }
                sw << line;
            }
        }
    endOfComment:
        if (sw.getSeekPos() > 0) {
            PLY_ASSERT(!this->docState.markdown);
            PLY_ASSERT(this->docState.markdownLoc >= 0);
            this->docState.markdown = sw.moveToString();
        }
        if (!gotAnyMarkdown && !gotNonGroupDirective) {
            // Allow empty markdown in member documentation (eg. Float2::x and y).
            // FIXME: Would be better to store the LinearLocation of the start of the last line of
            // the documentation string.
            this->docState.markdownLoc = token.linearLoc;
            PLY_ASSERT(!this->docState.markdown);
        }
    }

    virtual void onGotDeclaration(const cpp::grammar::Declaration& decl) override {
        PLY_ON_SCOPE_EXIT({
            if (this->docState.markdownLoc >= 0) {
                this->error(Error::StrayMarkdown, {}, this->docState.markdownLoc);
                this->docState.markdown = {};
                this->docState.markdownLoc = -1;
            }
        });

        ScopeInfo scopeInfo = this->getScopeInfo(this->scopeStack.numItems() - 1);
        if (this->docState.groupEntry) {
            if ((!scopeInfo.parentScope || scopeInfo.parentScope->type != SemaEntity::Class) &&
                !this->docState.addToClass)
                return;
        } else if (this->docState.markdownLoc >= 0) {
            if ((!scopeInfo.parentScope || scopeInfo.parentScope->type != SemaEntity::Class) &&
                !this->docState.addToClass) {
                this->error(Error::MarkdownOnlyValidWithinClass, {}, this->docState.markdownLoc);
                this->docState.markdown = {};
                this->docState.markdownLoc = -1;
                return;
            }
        } else {
            // Skip this declaration
            return;
        }
        PLY_ASSERT((scopeInfo.parentScope->type == SemaEntity::Class) || this->docState.addToClass);

        if (auto simple = decl.simple()) {
            Array<cpp::sema::SingleDeclaration> semaDecls =
                ply::cpp::semaFromParseTree(*simple.get(), this->parser->pp->visitedFiles);
            PLY_ASSERT(semaDecls.numItems() == simple->initDeclarators.numItems());
            for (u32 i = 0; i < semaDecls.numItems(); i++) {
                cpp::sema::SingleDeclaration& semaDecl = semaDecls[i];
                if (semaDecl.dcor.qid.nestedName.numItems() > 0) {
                    const cpp::grammar::QualifiedID& badQID = simple->initDeclarators[i].dcor.qid;
                    this->error(Error::NestedNamesNotSupported, badQID.toString(),
                                badQID.getFirstToken().linearLoc);
                    continue;
                }
                HybridString indexedName;
                if (auto ident = semaDecl.dcor.qid.unqual.identifier()) {
                    indexedName = ident->name.view();
                }
                Reference<SemaEntity> memberEnt = new SemaEntity;
                memberEnt->setParent(scopeInfo.parentScope);
                memberEnt->type = SemaEntity::Member;
                memberEnt->name = indexedName.view();
                if (scopeInfo.templateIdx >= 0) {
                    memberEnt->templateParams = this->semaScopeStack[scopeInfo.templateIdx];
                }
                if (scopeInfo.parentScope->type == SemaEntity::Class) {
                    if (!semaDecl.declSpecifierSeq.isEmpty()) {
                        // If there are no declSpecifiers, it's a ctor/dtor, so don't
                        // add it to the nameToChild map
                        scopeInfo.parentScope->nameToChild.insert(memberEnt);
                    }
                    scopeInfo.parentScope->childSeq.append(memberEnt);
                } else {
                    scopeInfo.parentScope->nameToChild.insert(memberEnt);
                    this->extractAPIResult->extractedAtNamespaceScope.append(memberEnt);
                }

                // Add function parameters as child SemaEntities
                if (semaDecl.dcor.prod) {
                    auto func = semaDecl.dcor.prod->function();
                    if (func) {
                        for (const cpp::sema::SingleDeclaration& param : func->params) {
                            PLY_ASSERT(param.dcor.qid.nestedName.isEmpty());
                            PLY_ASSERT(param.dcor.qid.unqual.identifier());
                            Reference<SemaEntity> sParam = new SemaEntity;
                            sParam->parent = memberEnt;
                            sParam->type = SemaEntity::FunctionParam;
                            sParam->name = param.dcor.qid.unqual.identifier()->name;
                            memberEnt->childSeq.append(sParam);
                            memberEnt->nameToChild.insert(sParam);
                        }
                    }
                }
                memberEnt->singleDecl = std::move(semaDecl);

                DocInfo* docInfo = nullptr;
                if (this->docState.addToClass) {
                    docInfo = this->docState.addToClass->docInfo;
                } else {
                    docInfo = scopeInfo.parentScope->docInfo;
                    PLY_ASSERT(docInfo->class_ == scopeInfo.parentScope);
                }

                DocInfo::Entry* entry = this->docState.groupEntry;
                if (!entry) {
                    entry = &docInfo->entries.append();
                    if (this->docState.groupDirectiveLoc >= 0) {
                        this->docState.groupEntry = entry;
                        if (this->docState.markdownLoc < 0) {
                            this->error(Error::GroupHasNoMarkdown, {},
                                        this->docState.groupDirectiveLoc);
                        } else {
                            entry->markdownDesc = std::move(this->docState.markdown);
                            this->docState.markdownLoc = -1;
                        }
                    } else {
                        PLY_ASSERT(this->docState.markdownLoc >= 0);
                        entry->markdownDesc = std::move(this->docState.markdown);
                        this->docState.markdownLoc = -1;
                    }
                    entry->categoryIndex = this->docState.categoryIndex;
                }

                DocInfo::Entry::Title* title = &entry->titles.append();
                title->member = memberEnt;
                // Set file and line number
                // FIXME: If the member is declared inside a macro or macros, get the file and line
                // containing the macro(s) instead
                cpp::ExpandedFileLocation fileLoc = cpp::expandFileLocation(
                    this->parser->pp->visitedFiles,
                    simple->initDeclarators[i].dcor.qid.getFirstToken().linearLoc);
                title->srcPath = fileLoc.srcFile->absPath;
                title->lineNumber = fileLoc.fileLoc.lineNumber;
            }
        }
    }

    virtual void gotMacroOrComment(cpp::Token token) override {
        if (token.type == cpp::Token::CStyleComment && token.identifier.startsWith("/*!")) {
            this->parseDocString(token);
        }
    }

    virtual bool handleError(Owned<cpp::BaseError>&& err) override {
        StringWriter sw;
        err->writeMessage(&sw, this->parser->pp->visitedFiles);
        this->extractAPIResult->addError(sw.moveToString());
        return true;
    }
};
PLY_REFLECT_ENUM(, APIExtractor::Error::Type)

void APIExtractor::Error::writeMessage(StringWriter* sw,
                                       const cpp::PPVisitedFiles* visitedFiles) const {
    if (this->linearLoc >= 0) {
        sw->format("{}: ", expandFileLocation(visitedFiles, this->linearLoc).toString());
    }
    *sw << "error: ";
    if (this->type >= APIExtractor::Error::BeginParseTitleError &&
        this->type < APIExtractor::Error::EndParseTitleError) {
        writeParseTitleError(
            sw, (ParseTitleError)(this->type - APIExtractor::Error::BeginParseTitleError),
            this->arg);
    } else {
        switch (this->type) {
            case APIExtractor::Error::AlreadyDefined: {
                sw->format("'{}' already defined\n", this->arg);
                break;
            }
            case APIExtractor::Error::StrayMarkdown: {
                sw->format("markdown text must be followed by a declaration\n");
                break;
            }
            case APIExtractor::Error::NestedNamesNotSupported: {
                sw->format(
                    "documentation comment on nested name \"{}\" is not currently supported\n",
                    this->arg);
                break;
            }
            case APIExtractor::Error::DirectivesMustBeAtStartOfDocumentationComment: {
                *sw << "directives must be at start of documentation comment\n";
                break;
            }
            case APIExtractor::Error::EmptyDirective: {
                *sw << "expected directive after '\\'\n";
                break;
            }
            case APIExtractor::Error::BadDirective: {
                sw->format("unrecognized directive '\\{}'\n", this->arg);
                break;
            }
            case APIExtractor::Error::DirectiveDoesNotTakeArguments: {
                sw->format("\\{} directive does not accept any arguments\n", this->arg);
                break;
            }
            case APIExtractor::Error::UnterminatedGroup: {
                *sw << "unterminated \\beginGroup directive\n";
                break;
            }
            case APIExtractor::Error::AlreadyInsideGroup: {
                *sw << "already insde a \\beginGroup directive\n";
                break;
            }
            case APIExtractor::Error::EndGroupOutsideGroup: {
                *sw << "\\endGroup must be preceded by a \\beginGroup directive\n";
                break;
            }
            case APIExtractor::Error::GroupHasNoMarkdown: {
                *sw << "\\beginGroup directive without markdown description\n";
                break;
            }
            case APIExtractor::Error::StrayMarkdownInGroup: {
                // FIXME: Log the location of the opening \\beginGroup block as additional
                // information
                *sw << "illegal markdown inside \\beginGroup block\n";
                break;
            }
            case APIExtractor::Error::EmptyDocumentationComment: {
                *sw << "empty documentation comment\n";
                break;
            }
            case APIExtractor::Error::DirectiveOnlyValidWithinClass: {
                sw->format("\\{} directive is only allowed inside a class\n", this->arg);
                break;
            }
            case APIExtractor::Error::MarkdownOnlyValidWithinClass: {
                *sw << "markdown is only supported for classes and class members\n";
                break;
            }
            case APIExtractor::Error::DirectiveNotValidWithinClass: {
                sw->format("\\{} directive not allowed inside a class\n", this->arg);
                break;
            }
            case APIExtractor::Error::ExpectedClassName: {
                sw->format("expected class name after \\{} directive\n", this->arg);
                break;
            }
            case APIExtractor::Error::UnexpectedAfterClassName: {
                sw->format("unexpected text after class name in \\{} directive\n", this->arg);
                break;
            }
            case APIExtractor::Error::ClassNotFound: {
                sw->format("class '{}' not found\n", this->arg);
                break;
            }
            default: {
                *sw << "error message not implemented!\n";
                break;
            }
        }
    }
}

void ExtractAPI_cook(cook::CookResult* cookResult_, TypedPtr) {
    cook::DependencyTracker* depTracker = cook::DependencyTracker::current();
    WebCookerIndex* userData = depTracker->userData.cast<WebCookerIndex>();
    PLY_ASSERT(userData->globalScope);

    // FIXME: implement safe cast
    PLY_ASSERT(cookResult_->job->id.type == &CookJobType_ExtractAPI);
    CookResult_ExtractAPI* extractAPIResult = static_cast<CookResult_ExtractAPI*>(cookResult_);

    APIExtractor visor;
    visor.semaScopeStack.append(userData->globalScope);
    visor.extractAPIResult = extractAPIResult;

    cpp::PPVisitedFiles visitedFiles;
    cpp::parsePlywoodSrcFile(NativePath::join(PLY_WORKSPACE_FOLDER, extractAPIResult->job->id.desc),
                             &visitedFiles, &visor);
}

cook::CookJobType CookJobType_ExtractAPI = {
    "extractAPI",
    TypeResolver<CookResult_ExtractAPI>::get(),
    nullptr,
    ExtractAPI_cook,
};

} // namespace docs
} // namespace ply

#include "codegen/CookResult_ExtractAPI.inl" //%%
