/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cpp/Grammar.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-web-cook-docs/Sema.h>

namespace ply {
namespace cpp {

// This is temporary code until the C++ parser parses expressions correctly
String tempExtractInitializer(const PPVisitedFiles* visitedFiles, LinearLocation startLoc,
                              LinearLocation endLoc) {
    if (!visitedFiles || startLoc < 0 || endLoc < 0)
        return {};
    // Note: These iterators may not be usable if the initializer began or ended with a macro.
    // What we really want is the topmost item on the include chain that is not a macro.
    auto startIter = visitedFiles->locationMap.findLastLessThan(startLoc + 1);
    auto endIter = visitedFiles->locationMap.findLastLessThan(endLoc + 1);
    if (startIter.getItem().includeChainIdx != endIter.getItem().includeChainIdx)
        return {};
    const cpp::PPVisitedFiles::IncludeChain& chain =
        visitedFiles->includeChains[startIter.getItem().includeChainIdx];
    if (chain.isMacroExpansion)
        return {};
    const cpp::PPVisitedFiles::SourceFile* srcFile = &visitedFiles->sourceFiles[chain.fileOrExpIdx];
    PLY_ASSERT(startLoc >= startIter.getItem().linearLoc);
    PLY_ASSERT(endLoc >= endIter.getItem().linearLoc);
    u32 startPos =
        safeDemote<u32>(startIter.getItem().offset + (startLoc - startIter.getItem().linearLoc));
    u32 endPos = safeDemote<u32>(endIter.getItem().offset + (endLoc - endIter.getItem().linearLoc));
    return srcFile->contents.view().subStr(startPos, endPos - startPos);
}

struct SemaConverter {
    const PPVisitedFiles* visitedFiles = nullptr;
    bool anyError = false;

    PLY_NO_INLINE sema::TemplateArg toSema(const grammar::TemplateArgumentWithComma& gArg) {
        sema::TemplateArg sArg;
        if (auto gTypeID = gArg.type.typeID()) {
            auto sTypeID = sArg.type.typeID().switchTo();
            sTypeID->declSpecifierSeq = this->toSema(gTypeID->declSpecifierSeq.view());
            sTypeID->abstractDcor = this->toSema(gTypeID->abstractDcor);
        } else if (auto gUnknown = gArg.type.unknown()) {
            auto sUnknown = sArg.type.unknown();
            sUnknown->expression = tempExtractInitializer(
                this->visitedFiles, gUnknown->startToken.linearLoc,
                gUnknown->startToken.linearLoc + gUnknown->startToken.identifier.numBytes);
        } else {
            this->anyError = true;
        }
        return sArg;
    }

    PLY_NO_INLINE sema::QualifiedID toSema(const grammar::QualifiedID& gQID) {
        sema::QualifiedID sQID;
        for (const grammar::NestedNameComponent& gNestedComp : gQID.nestedName) {
            if (auto gIdentOrTempl = gNestedComp.type.identifierOrTemplated()) {
                if (gIdentOrTempl->openAngled.isValid()) {
                    auto sTemplated = sQID.nestedName.append().templated().switchTo();
                    sTemplated->name = gIdentOrTempl->name.identifier;
                    for (const grammar::TemplateArgumentWithComma& gTemplateArg :
                         gIdentOrTempl->args) {
                        sTemplated->args.append(this->toSema(gTemplateArg));
                    }
                } else {
                    auto sIdentifier = sQID.nestedName.append().identifier().switchTo();
                    sIdentifier->name = gIdentOrTempl->name.identifier;
                }
            } else if (auto gDeclType = gNestedComp.type.declType()) {
                auto sDeclType = sQID.nestedName.append().declType().switchTo();
                sDeclType->expression = tempExtractInitializer(
                    this->visitedFiles,
                    gDeclType->openParen.linearLoc + gDeclType->openParen.identifier.numBytes,
                    gDeclType->closeParen.linearLoc);
            } else {
                this->anyError = true;
            }
        }
        if (auto gIdentifier = gQID.unqual.identifier()) {
            auto sIdentifier = sQID.unqual.identifier().switchTo();
            sIdentifier->name = gIdentifier->name.identifier;
        } else if (auto gTemplateID = gQID.unqual.templateID()) {
            auto sTemplateID = sQID.unqual.templateID().switchTo();
            sTemplateID->name = gTemplateID->name.identifier;
            for (const grammar::TemplateArgumentWithComma& gArg : gTemplateID->args) {
                sTemplateID->args.append(this->toSema(gArg));
            }
        } else if (auto gDeclType = gQID.unqual.declType()) {
            auto sDeclType = sQID.unqual.declType().switchTo();
            sDeclType->expression = tempExtractInitializer(
                this->visitedFiles,
                gDeclType->openParen.linearLoc + gDeclType->openParen.identifier.numBytes,
                gDeclType->closeParen.linearLoc);
        } else if (auto gDestructor = gQID.unqual.destructor()) {
            auto sDestructor = sQID.unqual.destructor().switchTo();
            sDestructor->name = gDestructor->name.identifier;
        } else if (auto gOperatorFunc = gQID.unqual.operatorFunc()) {
            auto sOperatorFunc = sQID.unqual.operatorFunc().switchTo();
            sOperatorFunc->punc = gOperatorFunc->punc.type;
            sOperatorFunc->punc2 = gOperatorFunc->punc2.type;
        } else if (auto gConversionFunc = gQID.unqual.conversionFunc()) {
            auto sConversionFunc = sQID.unqual.conversionFunc().switchTo();
            sConversionFunc->declSpecifierSeq =
                this->toSema(gConversionFunc->declSpecifierSeq.view());
            sConversionFunc->abstractDcor = this->toSema(gConversionFunc->abstractDcor);
        }
        return sQID;
    }

    PLY_NO_INLINE Array<sema::DeclSpecifier>
    toSema(ArrayView<const Owned<grammar::DeclSpecifier>> gDeclSpecifierSeq) {
        Array<sema::DeclSpecifier> sDeclSpecs;
        for (const grammar::DeclSpecifier* gDeclSpec : gDeclSpecifierSeq) {
            if (auto gKeyword = gDeclSpec->keyword()) {
                auto sKeyword = sDeclSpecs.append().keyword().switchTo();
                sKeyword->token = gKeyword->token.identifier;
            } else if (auto gTypeID = gDeclSpec->typeID()) {
                auto sTypeID = sDeclSpecs.append().typeID().switchTo();
                sTypeID->hasTypename = gTypeID->typename_.isValid();
                sTypeID->wasAssumed = gTypeID->wasAssumed;
                sTypeID->qid = this->toSema(gTypeID->qid);
            } else if (auto gTypeParam = gDeclSpec->typeParam()) {
                auto sTypeParam = sDeclSpecs.append().typeParam().switchTo();
                sTypeParam->hasEllipsis = gTypeParam->ellipsis.isValid();
            } else {
                this->anyError = true;
            }
        }
        return sDeclSpecs;
    }

    PLY_NO_INLINE sema::SingleDeclaration toSema(const grammar::ParamDeclarationWithComma& gParam) {
        sema::SingleDeclaration sSingle;
        sSingle.declSpecifierSeq = this->toSema(gParam.declSpecifierSeq.view());
        sSingle.dcor = this->toSema(gParam.dcor);
        if (auto gAssignment = gParam.init.assignment()) {
            auto sAssignment = sSingle.init.assignment().switchTo();
            if (auto gExpression = gAssignment->type.expression()) {
                sAssignment->type.unknown().switchTo()->expression = tempExtractInitializer(
                    this->visitedFiles, gExpression->start.linearLoc,
                    gExpression->end.linearLoc + gExpression->end.identifier.numBytes);
            } else if (auto gTypeID = gAssignment->type.typeID()) {
                auto sTypeID = sAssignment->type.typeID().switchTo();
                sTypeID->declSpecifierSeq = this->toSema(gTypeID->declSpecifierSeq.view());
                sTypeID->abstractDcor = this->toSema(gTypeID->abstractDcor);
            } else {
                this->anyError = true;
            }
        }
        return sSingle;
    }

    PLY_NO_INLINE Owned<sema::DeclaratorProduction>
    toSema(const grammar::DeclaratorProduction* gDcor) {
        if (!gDcor)
            return nullptr;
        auto sProd = Owned<sema::DeclaratorProduction>::create();
        if (auto gPointerTo = gDcor->type.pointerTo()) {
            auto sPointerTo = sProd->pointerTo().switchTo();
            sPointerTo->puncType = gPointerTo->punc.type;
            sPointerTo->target = this->toSema(gDcor->target);
        } else if (auto gFunction = gDcor->type.function()) {
            auto sFunction = sProd->function().switchTo();
            sFunction->target = this->toSema(gDcor->target);
            for (const grammar::ParamDeclarationWithComma& gParam : gFunction->params.params) {
                sFunction->params.append(this->toSema(gParam));
            }
            for (const Token& qualToken : gFunction->qualifiers.tokens) {
                sFunction->qualifiers.append(qualToken.type);
            }
        } else if (auto gQualifier = gDcor->type.qualifier()) {
            auto sQualifier = sProd->qualifier().switchTo();
            sQualifier->keyword = gQualifier->keyword.identifier;
            sQualifier->target = this->toSema(gDcor->target);
        } else if (auto gArrayOf = gDcor->type.arrayOf()) {
            auto sArrayOf = sProd->arrayOf().switchTo();
            sArrayOf->target = this->toSema(gDcor->target);
        } else if (auto gParenthesized = gDcor->type.parenthesized()) {
            sProd = this->toSema(gDcor->target);
        } else {
            this->anyError = true;
        }
        return sProd;
    }

    PLY_NO_INLINE sema::Declarator toSema(const grammar::Declarator& gDcor) {
        return {this->toSema(gDcor.prod), this->toSema(gDcor.qid)};
    }

    PLY_NO_INLINE Array<sema::SingleDeclaration>
    toSema(const grammar::Declaration::Simple& gSimple) {
        Array<sema::SingleDeclaration> sSingles;
        for (const grammar::InitDeclaratorWithComma& gInitDcor : gSimple.initDeclarators) {
            sema::SingleDeclaration& sSingle = sSingles.append();
            sSingle.declSpecifierSeq = this->toSema(gSimple.declSpecifierSeq.view());
            sSingle.dcor = this->toSema(gInitDcor.dcor);
            if (auto bitField = gInitDcor.init.bitField()) {
                sSingle.init.bitField().switchTo()->expression =
                    tempExtractInitializer(this->visitedFiles, bitField->expressionStart.linearLoc,
                                           bitField->expressionEnd.linearLoc +
                                               bitField->expressionEnd.identifier.numBytes);
            }
        }
        return sSingles;
    }
};

Array<sema::SingleDeclaration> semaFromParseTree(const grammar::Declaration::Simple& gSimple,
                                                 const PPVisitedFiles* visitedFiles) {
    SemaConverter conv;
    conv.visitedFiles = visitedFiles;
    Array<sema::SingleDeclaration> result = conv.toSema(gSimple);
    if (conv.anyError)
        return {};
    return result;
}

sema::SingleDeclaration semaFromParam(const grammar::ParamDeclarationWithComma& gParam,
                                      const PPVisitedFiles* visitedFiles) {
    SemaConverter conv;
    conv.visitedFiles = visitedFiles;
    sema::SingleDeclaration result = conv.toSema(gParam);
    if (conv.anyError)
        return {};
    return result;
}

sema::QualifiedID semaFromQID(const grammar::QualifiedID& gQID,
                              const PPVisitedFiles* visitedFiles) {
    SemaConverter conv;
    conv.visitedFiles = visitedFiles;
    sema::QualifiedID result = conv.toSema(gQID);
    if (conv.anyError)
        return {};
    return result;
}

Array<StringView> sema::QualifiedID::getSimplifiedComponents() const {
    Array<StringView> result;
    for (const NestedNameComponent& comp : this->nestedName) {
        if (auto ident = comp.identifier()) {
            result.append(ident->name);
        } else if (auto templated = comp.templated()) {
            result.append(templated->name);
        } else {
            return {};
        }
    }
    if (auto ident = this->unqual.identifier()) {
        result.append(ident->name);
    } else if (auto templated = this->unqual.templateID()) {
        result.append(templated->name);
    } else {
        return {};
    }
    return result;
}

} // namespace cpp
} // namespace ply

#include "codegen/Sema.inl" //%%
