/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Base.h>
#include <ply-runtime/Base.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>
#include <ply-runtime/process/Subprocess.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <web-sass/Sass.h>
#include <ply-cook/CookJob.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>
#include <web-documentation/Contents.h>

namespace ply {
namespace docs {
extern cook::CookJobType CookJobType_CopyStatic;
extern cook::CookJobType CookJobType_ExtractAPI;
extern cook::CookJobType CookJobType_ExtractPageMeta;
extern cook::CookJobType CookJobType_Page;
extern cook::CookJobType CookJobType_StyleSheetID;
void initCookJobTypes();
} // namespace docs
} // namespace ply

using namespace ply;

Array<Reference<cook::CookJob>> copyStaticFiles(cook::CookContext* ctx,
                                                StringView srcRoot) {
    Array<Reference<cook::CookJob>> copyJobs;
    for (WalkTriple& triple : FileSystem.walk(srcRoot)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            String relativeDir = Path.makeRelative(srcRoot, triple.dirPath);
            copyJobs.append(ctx->cook({&ply::docs::CookJobType_CopyStatic,
                                       Path.join(relativeDir, file.name)}));
        }
    }
    return copyJobs;
}

Array<String> getSourceFileKeys(StringView srcRoot) {
    Array<String> srcKeys;
    for (WalkTriple& triple : FileSystem.walk(srcRoot)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                // FIXME: Eliminate exclusions
                for (StringView exclude : {
                         "Sort.h",
                         "Functor.h",
                         "DirectoryWatcher_Mac.h",
                         "DirectoryWatcher_Win32.h",
                         "Heap.cpp",
                         "Pool.h",
                     }) {
                    if (file.name == exclude)
                        goto skipIt;
                }
                {
                    String relativeDir =
                        Path.makeRelative(Workspace.path, triple.dirPath);
                    srcKeys.append(Path.join(relativeDir, file.name));
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
    return srcKeys;
}

Reference<cook::CookJob> extractPageMetasFromFolder(cook::CookContext* ctx,
                                                    StringView relPath) {
    PLY_ASSERT(relPath.startsWith("/"));
    Reference<cook::CookJob> pageMetaJob = ctx->depTracker->getOrCreateCookJob(
        {&docs::CookJobType_ExtractPageMeta, PosixPath.join(relPath, "index")});
    Array<Reference<cook::CookJob>> childJobs;
    String absPath = Path.join(Workspace.path, "repos/plywood/docs", relPath.subStr(1));

    // By default, sort child pages by filename
    // The order can be overridden for each page using the <% childOrder %> tag
    Array<DirectoryEntry> allEntries;
    for (const DirectoryEntry& entry : FileSystem.listDir(absPath)) {
        allEntries.append(entry);
    }
    sort(allEntries, [](const DirectoryEntry& a, const DirectoryEntry& b) {
        return a.name < b.name;
    });

    // Add child entries
    for (const DirectoryEntry& entry : allEntries) {
        if (entry.isDir) {
            childJobs.append(
                extractPageMetasFromFolder(ctx, PosixPath.join(relPath, entry.name)));
        } else if (entry.name.endsWith(".md") && entry.name != "index.md") {
            StringView baseName = entry.name.shortenedBy(3);
            childJobs.append(ctx->cook({&docs::CookJobType_ExtractPageMeta,
                                        PosixPath.join(relPath, baseName)}));
        }
    }

    ctx->ensureCooked(pageMetaJob, AnyObject::bind(&childJobs));
    return pageMetaJob;
}

void visitPageMetas(
    const cook::CookJob* pageMetaJob,
    const Functor<void(const docs::CookResult_ExtractPageMeta*)>& visitor) {
    const docs::CookResult_ExtractPageMeta* pageMetaResult =
        pageMetaJob->castResult<docs::CookResult_ExtractPageMeta>();
    PLY_ASSERT(pageMetaResult);
    visitor(pageMetaResult);
    for (const cook::CookJob* childJob : pageMetaResult->childPages) {
        visitPageMetas(childJob, visitor);
    }
}

Owned<web::Contents> convertContents(const cook::CookJob* pageMetaJob) {
    Owned<web::Contents> dstNode = new web::Contents;
    const docs::CookResult_ExtractPageMeta* pageMetaResult =
        pageMetaJob->castResult<docs::CookResult_ExtractPageMeta>();
    PLY_ASSERT(pageMetaResult);

    // Set title
    dstNode->title = pageMetaResult->title;
    dstNode->linkDestination = pageMetaResult->getLinkDestination();
    for (const cook::CookJob* childMetaJob : pageMetaResult->childPages) {
        dstNode->children.append(convertContents(childMetaJob));
    }
    return dstNode;
}

int main() {
    ply::docs::initCookJobTypes();

    cook::DependencyTracker db;
    docs::WebCookerIndex* wci = new docs::WebCookerIndex;
    wci->globalScope = new docs::SemaEntity;
    db.userData = AnyOwnedObject::bind(wci);
    // String dbPath = Path.join(Workspace.path, "data/docsite-cache/depTracker.db");

    cook::CookContext ctx;
    ctx.depTracker = &db;
    ctx.beginCook();

    // Copy static files
    Array<Reference<cook::CookJob>> copyJobs =
        copyStaticFiles(&ctx, Path.join(Workspace.path, "repos/plywood/src/web/theme"));

    // Extract API documentation from the source code
    Array<Reference<cook::CookJob>> rootRefs;
    Array<String> srcKeys = getSourceFileKeys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/io"));
    srcKeys.extend(getSourceFileKeys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/container")));
    srcKeys.extend(getSourceFileKeys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/string")));
    srcKeys.extend(getSourceFileKeys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/filesystem")));
    srcKeys.extend(getSourceFileKeys(
        Path.join(Workspace.path, "repos/plywood/src/math/math/ply-math")));
    for (StringView srcKey : srcKeys) {
        rootRefs.append(ctx.cook({&ply::docs::CookJobType_ExtractAPI, srcKey}));
    }

    // Extract page metas
    Reference<cook::CookJob> contentsRoot = extractPageMetasFromFolder(&ctx, "/");

    // Cook all pages
    visitPageMetas(
        contentsRoot, [&](const docs::CookResult_ExtractPageMeta* pageMetaResult) {
            rootRefs.append(
                ctx.cook({&ply::docs::CookJobType_Page, pageMetaResult->job->id.desc}));
        });

    ctx.cookDeferred();

    // Save contents (FIXME: Skip this step if dependencies haven't changed)
    Array<Owned<web::Contents>> contents;
    {
        Owned<web::Contents> converted = convertContents(contentsRoot);
        web::Contents* home = contents.append(new web::Contents);
        home->title = "Home";
        home->linkDestination = "/";
        contents.moveExtend(converted->children);
    }
    {
        auto aRoot = pylon::exportObj(AnyObject::bind(&contents));
        MemOutStream mout;
        pylon::write(&mout, aRoot);
        FileSystem.makeDirsAndSaveTextIfDifferent(
            Path.join(Workspace.path, "data/docsite/contents.pylon"),
            mout.moveToString(), TextFormat::unixUTF8());
    }

    // Cook stylesheet
    rootRefs.append(ctx.cook({&ply::docs::CookJobType_StyleSheetID, {}}));

    db.setRootReferences(std::move(rootRefs));
    contentsRoot.clear();
    copyJobs.clear();
    ctx.endCook();
    return 0;
}
