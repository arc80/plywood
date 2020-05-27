/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-serve-docs/Core.h>
#include <ply-web-serve-docs/DocServer.h>
#include <pylon/Parse.h>
#include <pylon-reflect/Import.h>

namespace ply {
namespace web {

void dumpContents(StringWriter* sw, ArrayView<const Contents> children) {
    if (children.numItems > 0) {
        *sw << "<ul>\n";
        for (const Contents& child : children) {
            if (child.linkDestination) {
                sw->format("<li><a href=\"{}\">{}</a></li>\n", child.linkDestination, child.title);
            } else {
                sw->format("<li>{}</li>\n", child.title);
            }
            dumpContents(sw, child.children.view());
        }
        *sw << "</ul>\n";
    }
}

void DocServer::init(StringView dataRoot) {
    FileSystem* fs = FileSystem::native();

    this->dataRoot = dataRoot;
    this->contentsPath = NativePath::join(dataRoot, "contents.pylon");
    FileStatus contentsStatus = fs->getFileStatus(this->contentsPath);
    if (contentsStatus.result == FSResult::OK) {
        this->reloadContents();
        this->contentsModTime = contentsStatus.modificationTime;
    }
}

void DocServer::reloadContents() {
    FileSystem* fs = FileSystem::native();

    String contentsPylon = fs->loadText(this->contentsPath, TextFormat::unixUTF8());
    if (fs->lastResult() != FSResult::OK) {
        // FIXME: Log an error here
        return;
    }

    auto aRoot = pylon::Parser{}.parse(contentsPylon);
    if (!aRoot.isValid()) {
        // FIXME: Log an error here
        return;
    }

    pylon::importInto(TypedPtr::bind(&this->contents), aRoot);
}

void DocServer::serve(StringView requestPath, ResponseIface* responseIface) {
    FileSystem* fs = FileSystem::native();

    // Check if contents.pylon has been updated:
    double latestModTime = 0;
    FileStatus contentsStatus = fs->getFileStatus(this->contentsPath);
    if (contentsStatus.result == FSResult::OK) {
        if (contentsStatus.modificationTime != this->contentsModTime.load(MemoryOrder::Acquire)) {
            ply::LockGuard<ply::Mutex> guard{this->contentsMutex};
            if (contentsStatus.modificationTime !=
                this->contentsModTime.load(MemoryOrder::Relaxed)) {
                this->reloadContents();
            }
            this->contentsModTime = contentsStatus.modificationTime;
        }
    }

    if (!this->contents) {
        responseIface->respondGeneric(ResponseCode::InternalError);
        return;
    }

    // Load page
    if (NativePath::isAbsolute(requestPath)) {
        responseIface->respondGeneric(ResponseCode::NotFound);
        return;
    }
    String absPath = NativePath::join(this->dataRoot, "pages", requestPath);
    ExistsResult exists = FileSystem::native()->exists(absPath);
    if (exists == ExistsResult::Directory) {
        absPath = NativePath::join(absPath, "index.html");
    } else {
        absPath += ".html";
    }
    String pageHtml =
        fs->loadText(NativePath::join(this->dataRoot, "pages", absPath), TextFormat::unixUTF8());
    StringViewReader svr{pageHtml};
    String pageTitle = svr.readView<fmt::Line>().trim(isWhite);
    if (fs->lastResult() != FSResult::OK) {
        responseIface->respondGeneric(ResponseCode::NotFound);
        return;
    }

    OutStream* outs = responseIface->respondWithStream(ResponseCode::OK);
    StringWriter* sw = outs->strWriter();
    *sw << "Content-Type: text/html\r\n\r\n";
    sw->format(R"#(<!DOCTYPE html>
<html>
<head>
<title>{}</title>
)#",
               pageTitle);
    *sw << R"#(<link href="/static/stylesheet.css" rel="stylesheet" type="text/css" />
<script>
var highlighted = null;
function highlight(elementID) {
    if (highlighted) {
        highlighted.style.background = "";
    }
    highlighted = document.getElementById(elementID);
    if (highlighted) {
        highlighted.style.background = "#ffffa0";
    }
}
window.onload = function() { 
    highlight(location.hash.substr(1));
    var defTitles = document.getElementsByClassName("defTitle");
    for (var i = 0; i < defTitles.length; i++) {
        defTitles[i].onmouseenter = function(e) {
            var linkElems = e.target.getElementsByClassName("headerlink");
            for (var j = 0; j < linkElems.length; j++) {
                linkElems[j].style.visibility = "visible";
            }
        }
        defTitles[i].onmouseleave = function(e) {
            var linkElems = e.target.getElementsByClassName("headerlink");
            for (var j = 0; j < linkElems.length; j++) {
                linkElems[j].style.visibility = "hidden";
            }
        }
    }
}
window.onhashchange = function() { 
    highlight(location.hash.substr(1));
}
</script>
</head>
<body>
<div class="container">
<div id="siteTitle">
<a href="/"><img src="/static/logo.svg" id="logo"/></a>
<a href="https://www.patreon.com/preshing"><img src="/static/patron-button.svg" id="patron"></a>
<a href="https://github.com/arc80/plywood"><img src="/static/github-button.svg" id="github"></a>
</div>
<div class="sidebar">
<div class="inner">
)#";
    dumpContents(sw, this->contents.view());
    sw->format(R"(
</div></div>
<div class="content">
<article>
<h1>{}</h1>
)",
               pageTitle);
    *sw << svr.viewAvailable();
    *sw << R"(</article>
</div>
</div>
</body>
</html>
)";
}

} // namespace web
} // namespace ply
