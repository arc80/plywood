/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/CookResult_Page.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>
#include <web-markdown/Markdown.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-web-cook-docs/SemaToString.h>
#include <ply-runtime/io/text/FileLocationMap.h> // This should be moved to a different module

namespace ply {
namespace docs {

extern cook::CookJobType CookJobType_Page;

void visitAll(markdown::Node* node, const LambdaView<bool(markdown::Node*)>& callback) {
    if (callback(node)) {
        for (markdown::Node* child : node->children) {
            visitAll(child, callback);
        }
    }
}

SemaEntity* resolveClassScope(StringView classScopeText) {
    cook::CookContext* ctx = cook::CookContext::current();
    WebCookerIndex* wci = ctx->depTracker->userData.safeCast<WebCookerIndex>();
    Array<StringView> comps = classScopeText.splitByte(':');
    return wci->globalScope->lookupChain(comps.view());
}

String getLinkDestination(const SemaEntity* targetSema) {
    cook::DependencyTracker* depTracker = cook::DependencyTracker::current();
    WebCookerIndex* wci = depTracker->userData.safeCast<WebCookerIndex>();
    String memberSuffix;
    if (targetSema->type == SemaEntity::Member) {
        memberSuffix = StringView{"#"} + targetSema->name;
        targetSema = targetSema->parent;
        if (targetSema->type != SemaEntity::Class)
            return {};
    }
    auto iter = wci->extractPageMeta.findFirstGreaterOrEqualTo(targetSema->name);
    for (; iter.isValid() && iter.getItem()->semaEnt->name == targetSema->name; iter.next()) {
        if (iter.getItem()->semaEnt == targetSema) {
            return iter.getItem()->linkDestination + memberSuffix;
        }
    }
    return {};
}

HybridString wrapWithLink(StringView html, const SemaEntity* targetSema) {
    String linkDestination = getLinkDestination(targetSema);
    if (linkDestination) {
        return String::format("<a href=\"{}\">{}</a>", fmt::XMLEscape{linkDestination}, html);
    } else {
        return html;
    }
}

struct LookupContext {
    SemaEntity* forClass = nullptr;
    ArrayView<SemaEntity* const> semaGroup;

    bool shouldShowLink(SemaEntity* semaToLink) const {
        // Avoid unnecessary/unhelpful/"noisy" links
        if (semaToLink == this->forClass)
            return false;
        for (SemaEntity* checkSema : this->semaGroup) {
            if (semaToLink == checkSema || semaToLink->parent == checkSema)
                return false;
        }
        return true;
    }
};

String getLinkDestinationFromSpan(StringView codeSpanText, const LookupContext& lookupCtx) {
    if (codeSpanText.endsWith("()")) {
        codeSpanText = codeSpanText.shortenedBy(2);
    }
    Array<StringView> nameComps = codeSpanText.splitByte(':');
    if (nameComps.isEmpty())
        return {};
    PLY_ASSERT(!codeSpanText.startsWith(":")); // Not supported yet
    for (SemaEntity* fromSema : lookupCtx.semaGroup) {
        SemaEntity* foundSema = fromSema->lookupChain(nameComps.view());
        if (foundSema) {
            if (!lookupCtx.shouldShowLink(foundSema))
                return {};
            return getLinkDestination(foundSema);
        }
    }
    if (lookupCtx.forClass) {
        // Try class itself last
        SemaEntity* foundSema = lookupCtx.forClass->lookupChain(nameComps.view());
        if (foundSema) {
            if (!lookupCtx.shouldShowLink(foundSema))
                return {};
            return getLinkDestination(foundSema);
        }
    }
    // No potential link destination found
    return {};
}

Owned<markdown::Node> parseMarkdown(StringView markdown, const LookupContext& lookupCtx) {
    using namespace markdown;
    Owned<Node> document = parse(markdown);
    WebCookerIndex* wci = cook::DependencyTracker::current()->userData.safeCast<WebCookerIndex>();
    visitAll(document, [&](Node* node) -> bool {
        if (node->type == Node::Link) {
            // Fixup link destinations
            if (!node->text.startsWith("/") && node->text.findByte(':') < 0) {
                s32 anchorPos = node->text.findByte('#');
                if (anchorPos < 0) {
                    anchorPos = node->text.numBytes;
                }
                StringView linkID = node->text.left(anchorPos);
                if (linkID) {
                    auto iter = wci->linkIDMap.findFirstGreaterOrEqualTo(linkID);
                    if (iter.isValid() && iter.getItem()->linkID == linkID) {
                        node->text =
                            iter.getItem()->getLinkDestination() + node->text.subStr(anchorPos);
                    }
                }
            }
            return false;
        } else if (node->type == Node::CodeSpan) {
            String linkDestination = getLinkDestinationFromSpan(node->text, lookupCtx);
            if (!linkDestination)
                return false;
            Node* parent = node->parent;
            s32 i = findItem(parent->children.view(), node);
            PLY_ASSERT(i >= 0);
            Owned<Node> movedNode = std::move(parent->children[i]);
            PLY_ASSERT(movedNode == node);
            Owned<Node> link = new Node{nullptr, Node::Link};
            link->text = std::move(linkDestination);
            link->parent = node;
            link->children.append(std::move(movedNode));
            parent->children[i] = std::move(link);
            node->parent = link;
            return false;
        } else if (node->type == Node::Heading) {
            if (node->children.numItems() == 1) {
                Node* linkNode = node->children[0];
                if (linkNode->type == Node::Link && linkNode->text.startsWith("#")) {
                    // Convert this to an anchor
                    node->id = linkNode->text.subStr(1);
                    Array<Owned<Node>> promoteChildren = std::move(linkNode->children);
                    node->children = std::move(promoteChildren);
                }
            }
        }
        return true;
    });
    return document;
}

String convertMarkdownToHTML(StringView markdown, const LookupContext& lookupCtx) {
    Owned<markdown::Node> document = parseMarkdown(markdown, lookupCtx);
    StringWriter sw;
    markdown::HTMLOptions options;
    options.childAnchors = true;
    convertToHTML(&sw, document, options);
    return sw.moveToString();
}

void dumpMemberTitle(const DocInfo::Entry::Title& title, StringWriter& htmlWriter,
                     bool prependClassName, const LookupContext& lookupCtx) {
    const SemaEntity* templateParams = title.member->templateParams;
    const cpp::sema::SingleDeclaration* singleDecl = &title.member->singleDecl;

    bool inRootDeclarator = false;
    using C = cpp::sema::Stringifier::Component;
    if (templateParams) {
        htmlWriter << "<code class=\"template\">template &lt;";
        bool first = true;
        for (const SemaEntity* param : templateParams->childSeq) {
            if (!first) {
                htmlWriter << ", ";
            }
            first = false;
            for (const C& comp : cpp::sema::toStringComps(param->singleDecl, nullptr, false)) {
                htmlWriter << fmt::XMLEscape{comp.text};
            }
        }
        htmlWriter << "&gt;</code><br>\n";
    }
    htmlWriter << "<code>";
    for (const C& comp : cpp::sema::toStringComps(*singleDecl, title.member, prependClassName)) {
        if (comp.type == C::BeginRootDeclarator) {
            htmlWriter << "<strong>";
            inRootDeclarator = true;
        } else if (comp.type == C::EndRootDeclarator) {
            htmlWriter << "</strong>";
            inRootDeclarator = false;
        } else {
            String linkDestination;
            if (!inRootDeclarator && comp.sema) {
                PLY_ASSERT(comp.type == C::KeywordOrIdentifier);
                if (lookupCtx.shouldShowLink(comp.sema)) {
                    linkDestination = getLinkDestination(comp.sema);
                }
            }
            if (linkDestination) {
                htmlWriter.format("<a href=\"{}\">", fmt::XMLEscape{linkDestination});
            }
            htmlWriter << fmt::XMLEscape{comp.text};
            if (linkDestination) {
                htmlWriter << "</a>";
            }
        }
    }
    htmlWriter << "</code>\n";
}

void dumpBaseClasses(StringWriter& htmlWriter, SemaEntity* classEnt,
                     const LookupContext& lookupCtx) {
    // Dump base classes
    for (const cpp::sema::QualifiedID& qid : classEnt->baseClasses) {
        Array<StringView> comps = qid.getSimplifiedComponents();
        SemaEntity* baseEnt = classEnt->lookupChain(comps.view());
        if (!baseEnt)
            continue;
        if (!baseEnt->docInfo)
            continue;
        if (!baseEnt->docInfo->entries)
            continue;

        htmlWriter.format(
            "<h2>Members Inherited From {}</h2>\n",
            wrapWithLink(String::format("<code>{}</code>", baseEnt->getQualifiedID()), baseEnt));
        htmlWriter << "<ul>\n";
        for (const DocInfo::Entry& entry : baseEnt->docInfo->entries) {
            for (const DocInfo::Entry::Title& title : entry.titles) {
                htmlWriter << "<li>";
                dumpMemberTitle(title, htmlWriter, true, {baseEnt, lookupCtx.semaGroup});
                htmlWriter << "</li>\n";
            }
        }
        htmlWriter << "</ul>\n";

        dumpBaseClasses(htmlWriter, baseEnt, {baseEnt, lookupCtx.semaGroup});
    }
};

void dumpExtractedMembers(StringWriter& htmlWriter, SemaEntity* classEnt) {
    PLY_ASSERT(classEnt);
    PLY_ASSERT(classEnt->type == SemaEntity::Class);
    const DocInfo* docInfo = classEnt->docInfo;
    PLY_ASSERT(docInfo);

    auto dumpMemberEntry = [&](const DocInfo::Entry& entry, bool prependClassName) {
        Array<SemaEntity*> semaGroup;
        for (const DocInfo::Entry::Title& title : entry.titles) {
            semaGroup.append(title.member);
        }
        LookupContext lookupCtx{classEnt, semaGroup.view()};

        htmlWriter << "<dt>";
        for (const DocInfo::Entry::Title& title : entry.titles) {
            String anchor;
            String permalink;
            if (title.member->name) {
                htmlWriter.format("<div class=\"defTitle anchored\"><span class=\"anchor\" "
                                  "id=\"{}\">&nbsp;</span>",
                                  fmt::XMLEscape{title.member->name});
                /*
                permalink =
                    String::format(" <a class=\"headerlink\" href=\"#{}\" title=\"Permalink to "
                                   "this definition\">[link]</a>",
                                   fmt::XMLEscape{title.member->name});
                */
            } else {
                htmlWriter.format("<div class=\"defTitle\"{}>", anchor);
            }

            dumpMemberTitle(title, htmlWriter, prependClassName, lookupCtx);

            // Close <code> tag, write optional permalink & source code link, close <div>
            PLY_ASSERT(title.srcPath.startsWith(NativePath::normalize(PLY_WORKSPACE_FOLDER)));
            String srcPath = PosixPath::from<NativePath>(
                NativePath::makeRelative(PLY_WORKSPACE_FOLDER, title.srcPath));
#if WEBCOOKDOCS_LINK_TO_GITHUB
            // FIXME: Link to a specific commit
            StringView srcLinkPrefix = "https://github.com/arc80/plywood/tree/main/";
#else
            StringView srcLinkPrefix = "/file/";
#endif
            String srcLocation =
                String::format(" <a class=\"headerlink\" href=\"{}{}#L{}\" "
                               "title=\"Go to source code\">[code]</a>",
                               srcLinkPrefix, fmt::XMLEscape{srcPath}, title.lineNumber);
            htmlWriter.format("{}{}</div>\n", permalink, srcLocation);
        }
        htmlWriter << "</dt>\n";
        htmlWriter << "<dd>\n";
        htmlWriter << convertMarkdownToHTML(entry.markdownDesc, lookupCtx);
        htmlWriter << "</dd>\n";
    };

    // Dump data members
    bool wroteSectionHeader = false;
    for (const DocInfo::Entry& entry : docInfo->entries) {
        if (!entry.titles[0].member->isFunction()) {
            if (!wroteSectionHeader) {
                htmlWriter << "<h2>Data Members</h2>\n";
                htmlWriter << "<dl>\n";
                wroteSectionHeader = true;
            }
            dumpMemberEntry(entry, false);
        }
    }
    if (wroteSectionHeader) {
        htmlWriter << "</dl>\n\n";
    }

    // Dump member functions in categories first
    for (u32 c = 0; c < docInfo->categories.numItems(); c++) {
        wroteSectionHeader = false;
        for (const DocInfo::Entry& entry : docInfo->entries) {
            if (entry.titles[0].member->isFunction() && entry.categoryIndex == c) {
                if (!wroteSectionHeader) {
                    htmlWriter.format("<h2>{}</h2>\n", docInfo->categories[c].desc);
                    htmlWriter << "<dl>\n";
                    wroteSectionHeader = true;
                }
                dumpMemberEntry(entry, true);
            }
        }
    }

    // Dump uncategorized member functions
    wroteSectionHeader = false;
    for (const DocInfo::Entry& entry : docInfo->entries) {
        if (entry.titles[0].member->isFunction() && entry.categoryIndex < 0) {
            if (!wroteSectionHeader) {
                htmlWriter << "<h2>Member Functions</h2>\n";
                htmlWriter << "<dl>\n";
                wroteSectionHeader = true;
            }
            dumpMemberEntry(entry, true);
        }
    }
    if (wroteSectionHeader) {
        htmlWriter << "</dl>\n\n";
    }

    dumpBaseClasses(htmlWriter, classEnt, {classEnt, {}});
}

//---------------------------

u128 getClassHash(StringView classFQID) {
    cook::DependencyTracker* depTracker = cook::DependencyTracker::current();
    WebCookerIndex* wci = depTracker->userData.cast<WebCookerIndex>();
    SemaEntity* classEnt = wci->globalScope->lookupChain(classFQID.splitByte(':').view());
    if (!classEnt || classEnt->type != SemaEntity::Class)
        return {};
    return classEnt->hash;
}

extern cook::DependencyType DependencyType_ExtractedClassAPI;

struct Dependency_ExtractedClassAPI : cook::Dependency {
    String classFQID;
    u128 classHash;

    PLY_INLINE Dependency_ExtractedClassAPI(StringView classFQID) : classFQID{classFQID} {
        this->classHash = getClassHash(classFQID);
    }
};

cook::DependencyType DependencyType_ExtractedClassAPI{
    // hasChanged
    [](cook::Dependency* dep_, cook::CookResult*, TypedPtr) -> bool { //
        // FIXME: Implement safe cast
        Dependency_ExtractedClassAPI* depECA = static_cast<Dependency_ExtractedClassAPI*>(dep_);
        return depECA->classHash != getClassHash(depECA->classFQID);
    },
};

//---------------------------

void Page_cook(cook::CookResult* cookResult_, TypedPtr) {
    cook::CookContext* ctx = cook::CookContext::current();
    PLY_ASSERT(cookResult_->job->id.type == &CookJobType_Page);
    auto pageResult = static_cast<CookResult_Page*>(cookResult_);

    CookResult_ExtractPageMeta* extractMetaResult = static_cast<CookResult_ExtractPageMeta*>(
        ctx->getAlreadyCookedResult({&CookJobType_ExtractPageMeta, pageResult->job->id.desc}));

    String pageSrcPath = extractMetaResult->getMarkdownPath();
    Owned<InStream> ins = pageResult->openFileAsDependency(pageSrcPath);
    if (!ins) {
        // FIXME: Shouldn't create CookJobs for pages that don't exist
        if (NativePath::split(pageSrcPath).second != "index.md") {
            pageResult->addError(String::format("Unable to open \"{}\"\n", pageSrcPath));
        }
        return;
    }
    String src = FileIOWrappers::loadTextAutodetect(std::move(ins)).first;
    FileLocationMap srcFileLocMap = FileLocationMap::fromView(src);
    StringViewReader sr{src};

    // Extract liquid tags
    StringWriter sw;
    StringWriter htmlWriter;
    Array<String> childPageNames;
    String classScopeText;
    // FIXME: don't hardcode classScope
    WebCookerIndex* wci = ctx->depTracker->userData.safeCast<WebCookerIndex>();
    SemaEntity* classScope = wci->globalScope->lookup({"ply"});
    bool inMembers = false;
    auto flushMarkdown = [&] {
        String page = sw.moveToString();
        htmlWriter << convertMarkdownToHTML(page, {classScope, {}});
        sw = StringWriter{};
    };
    extractLiquidTags(&sw, &sr, [&](StringView tag, StringView section) {
        StringViewReader svr{section};
        svr.parse<fmt::Whitespace>();
        StringView command = svr.readView(fmt::Identifier{});
        svr.parse<fmt::Whitespace>();
        if (command == "title" || command == "linkID" || command == "synopsis" ||
            command == "childOrder") {
            // Handled by CookResult_ExtractPageMeta
        } else if (command == "note") {
            flushMarkdown();
            htmlWriter
                << "<div class=\"note\"><img src=\"/static/info-icon.svg\" class=\"icon\"/>\n";
            htmlWriter << convertMarkdownToHTML(svr.viewAvailable(), {classScope, {}});
            htmlWriter << "</div>\n";
        } else if (command == "member") {
            flushMarkdown();
            if (!inMembers) {
                htmlWriter << "<dl>\n";
                inMembers = true;
            } else {
                htmlWriter << "</dd>\n";
            }
            htmlWriter << "<dt><code>";
            // FIXME: handle errors here
            Array<TitleSpan> spans = parseTitle(svr.viewAvailable().rtrim(isWhite),
                                                [](ParseTitleError, StringView, const char*) {});
            writeAltMemberTitle(htmlWriter, spans.view(), {classScope, {}},
                                getLinkDestinationFromSpan);
            htmlWriter << "</code></dt>\n";
            htmlWriter << "<dd>\n";
        } else if (command == "endMembers") {
            if (inMembers) {
                flushMarkdown();
                htmlWriter << "</dd>\n";
                htmlWriter << "</dl>\n";
                inMembers = false;
            } else {
                PLY_ASSERT(0); // FIXME: Handle gracefully
            }
        } else if (command == "setClassScope") {
            classScopeText = svr.viewAvailable().trim(isWhite);
            classScope = resolveClassScope(classScopeText);
            if (!classScope) {
                FileLocation srcFileLoc =
                    srcFileLocMap.getFileLocation(safeDemote<u32>(tag.bytes - src.bytes));
                pageResult->addError(String::format("{}({}, {}): error: class '{}' not found\n",
                                                    pageSrcPath, srcFileLoc.lineNumber,
                                                    srcFileLoc.columnNumber, classScopeText));
            } else {
                if (classScope->docInfo->classMarkdownDesc) {
                    htmlWriter << convertMarkdownToHTML(classScope->docInfo->classMarkdownDesc,
                                                        {classScope, {}});
                }
            }
        } else if (command == "dumpExtractedMembers") {
            flushMarkdown();
            String classFQID = svr.viewAvailable().trim(isWhite);
            if (classFQID != classScopeText) {
                FileLocation srcFileLoc =
                    srcFileLocMap.getFileLocation(safeDemote<u32>(tag.bytes - src.bytes));
                pageResult->addError(
                    String::format("{}({}, {}): error: dumpExtractedMembers tag '{}' does not "
                                   "match earlier setClassScope tag '{}'\n",
                                   pageSrcPath, srcFileLoc.lineNumber, srcFileLoc.columnNumber,
                                   classFQID, classScopeText));
            }
            pageResult->dependencies.append(new Dependency_ExtractedClassAPI{classFQID});
            SemaEntity* classEnt = wci->globalScope->lookupChain(classFQID.splitByte(':').view());
            if (!classEnt) {
                // FIXME: It would be cool to set the columnNumber to the exact location of the
                // class name within the liquid tag, but that will require a way to map offsets
                // in the liquid tag to offsets in the file that accounts for escape characters.
                // (Although there aren't any escape chracters in liquid tags yet...) For now,
                // we'll just use the location of the tag itself.
                FileLocation srcFileLoc =
                    srcFileLocMap.getFileLocation(safeDemote<u32>(tag.bytes - src.bytes));
                pageResult->addError(String::format("{}({}, {}): error: class '{}' not found\n",
                                                    pageSrcPath, srcFileLoc.lineNumber,
                                                    srcFileLoc.columnNumber, classFQID));
                htmlWriter.format("<div class=\"error\">Error: Class \"{}\" not found</div>",
                                  fmt::XMLEscape{classFQID});
                return;
            }
            dumpExtractedMembers(htmlWriter, classEnt);
        } else if (command == "html") {
            flushMarkdown();
            htmlWriter << svr.viewAvailable();
        } else {
            FileLocation srcFileLoc =
                srcFileLocMap.getFileLocation(safeDemote<u32>(tag.bytes - src.bytes));
            pageResult->addError(String::format("{}({}, {}): error: unrecognized tag '{}'\n",
                                                pageSrcPath, srcFileLoc.lineNumber,
                                                srcFileLoc.columnNumber, command));
        }
    });

    // Save page html with single line title header
    // FIXME: Implement strategy to delete orphaned HTML files
    flushMarkdown();
    String finalHtml = String::format("{}\n", extractMetaResult->title) + htmlWriter.moveToString();
    PLY_ASSERT(pageResult->job->id.desc.startsWith("/"));
    String htmlPath = NativePath::join(PLY_WORKSPACE_FOLDER, "data/docsite/pages",
                                       pageResult->job->id.desc.subStr(1) + ".html");
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(htmlPath, finalHtml,
                                                         TextFormat::unixUTF8());
}

cook::CookJobType CookJobType_Page = {
    "page",
    TypeResolver<CookResult_Page>::get(),
    nullptr,
    Page_cook,
};

} // namespace docs
} // namespace ply

#include "codegen/CookResult_Page.inl" //%%
