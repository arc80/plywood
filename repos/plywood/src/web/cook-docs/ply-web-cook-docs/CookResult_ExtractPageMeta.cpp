/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace docs {

extern cook::CookJobType CookJobType_ExtractPageMeta;

void cook_ExtractPageMeta(cook::CookResult* cookResult_, TypedPtr jobArg) {
    cook::CookContext* ctx = cook::CookContext::current();
    WebCookerIndex* wci = ctx->depTracker->userData.safeCast<WebCookerIndex>();
    PLY_ASSERT(cookResult_->job->id.type == &CookJobType_ExtractPageMeta);
    auto extractPageMetaResult = static_cast<CookResult_ExtractPageMeta*>(cookResult_);

    // Take ownership of child page list
    // FIXME: Add dependency on this list!
    auto* childPagesInFolder = jobArg.cast<Array<Reference<cook::CookJob>>>();
    if (childPagesInFolder) {
        for (cook::CookJob* job : *childPagesInFolder) {
            PLY_ASSERT(job->castResult<CookResult_ExtractPageMeta>());
            PLY_UNUSED(job);
        }
        extractPageMetaResult->childPages = std::move(*childPagesInFolder);
    }

    {
        // Set default page title && linkID
        auto splitPath = PosixPath::split(cookResult_->job->id.desc);
        if (splitPath.second == "index") {
            extractPageMetaResult->title = PosixPath::split(splitPath.first).second;
        } else {
            extractPageMetaResult->title = splitPath.second;
        }
        extractPageMetaResult->linkID = extractPageMetaResult->title;
    }

    Owned<InStream> ins =
        extractPageMetaResult->openFileAsDependency(extractPageMetaResult->getMarkdownPath());
    if (ins) {
        extractPageMetaResult->markdownExists = true;

        String src = FileIOWrappers::loadTextAutodetect(std::move(ins)).first;
        ViewInStream ins{src};

        // Extract liquid tags
        MemOutStream mout; // Note: This could be some kind of "null writer" if such a thing existed
        extractLiquidTags(&mout, &ins, [&](StringView tag, StringView section) {
            ViewInStream vins{section};
            vins.parse<fmt::Whitespace>();
            StringView command = vins.readView(fmt::Identifier{});
            vins.parse<fmt::Whitespace>();
            if (command == "title") {
                extractPageMetaResult->title = vins.parse(fmt::QuotedString{});
            } else if (command == "linkID") {
                extractPageMetaResult->linkID = vins.viewAvailable().rtrim(isWhite);
            } else if (command == "childOrder") {
                // Override child page order
                Array<Reference<cook::CookJob>> origOrder =
                    std::move(extractPageMetaResult->childPages);
                for (;;) {
                    vins.parse<fmt::Whitespace>();
                    StringView childID = vins.readView(fmt::Identifier{});
                    if (!childID)
                        break;
                    s32 childIndex = find(origOrder.view(), [&](const cook::CookJob* child) {
                        return child->castResult<CookResult_ExtractPageMeta>()->linkID == childID;
                    });
                    if (childIndex < 0) {
                        // FIXME: Dump file/line number
                        extractPageMetaResult->addError(
                            String::format("'{}' is not a child page\n", childID));
                    } else {
                        extractPageMetaResult->childPages.append(std::move(origOrder[childIndex]));
                        origOrder.eraseQuick(childIndex);
                    }
                }
                if (origOrder.numItems() > 0) {
                    // FIXME: Dump file/line number
                    Array<String> childIDs;
                    for (const cook::CookJob* child : origOrder) {
                        childIDs.append(String::format(
                            "'{}'", child->castResult<CookResult_ExtractPageMeta>()->linkID));
                    }
                    extractPageMetaResult->addError(String::format(
                        "child pages not listed: {}",
                        StringView{", "}.join(Array<StringView>{childIDs.view()}.view())));
                }
            } else if (command == "dumpExtractedMembers") {
                String classFQID = vins.viewAvailable().trim(isWhite);
                Array<StringView> components = classFQID.splitByte(':');
                SemaEntity* ent = wci->globalScope->lookupChain(components.view());
                if (!ent)
                    return; // Ignore here; error will be logged by CookResult_Page
                auto pair = Owned<SymbolPagePair>::create();
                pair->semaEnt = ent;
                pair->linkDestination = extractPageMetaResult->getLinkDestination();
                pair->addToIndex();
                extractPageMetaResult->symbolPagePairs.append(std::move(pair));
            } else if (command == "synopsis") {
                extractPageMetaResult->synopsis = vins.viewAvailable();
            }
        });
    }

    // FIXME: Check for duplicate linkIDs
    wci->linkIDMap.insert(extractPageMetaResult);
}

cook::CookJobType CookJobType_ExtractPageMeta = {
    "extractPageMeta",
    TypeResolver<CookResult_ExtractPageMeta>::get(),
    TypeResolver<Array<Reference<cook::CookJob>>>::get(),
    cook_ExtractPageMeta,
};

} // namespace docs
} // namespace ply

#include "codegen/CookResult_ExtractPageMeta.inl" //%%
