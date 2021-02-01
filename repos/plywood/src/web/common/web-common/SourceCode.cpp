/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/SourceCode.h>

namespace ply {
namespace web {

PLY_NO_INLINE void SourceCode::serve(const SourceCode* params, StringView requestPath,
                                     ResponseIface* responseIface) {
    // FIXME: Use FileSystem_Virtual and make really, really sure an adversary can't read files
    // outside the rootDir
    String normRequestPath = NativePath::normalize(requestPath.ltrim(NativePath::isSepByte));
    Owned<InStream> ins =
        FileSystem::native()
            ->openTextForReadAutodetect(NativePath::join(params->rootDir, normRequestPath))
            .first;
    if (!ins) {
        // file could not be loaded
        responseIface->respondGeneric(ResponseCode::NotFound);
        return;
    }

    OutStream* outs = responseIface->beginResponseHeader(ResponseCode::OK);
    *outs << "Content-Type: text/html\r\n\r\n";
    responseIface->endResponseHeader();
    outs->format(R"#(<!DOCTYPE html>
<html>
<head>
<title>{}</title>
)#",
                 fmt::XMLEscape{NativePath::split(normRequestPath).second});
    *outs << R"#(<link href="/static/stylesheet.css" rel="stylesheet" type="text/css" />
<script>
var highlighted = null;
function highlight(elementID) {
    if (highlighted) {
        highlighted.style.background = "";
    }
    highlighted = document.getElementById("LC" + elementID);
    if (highlighted) {
        highlighted.style.background = "#ffffa0";
    }
}
window.onload = function() { 
    highlight(location.hash.substr(2));
}
window.onhashchange = function() { 
    highlight(location.hash.substr(2));
}
</script>
</head>
<body>
<div class="container">
<table>
<tbody>
)#";
    u32 lineNumber = 1;
    while (String line = ins->readString<fmt::Line>()) {
        outs->format("<tr><td id=\"L{}\">{}</td><td id=\"LC{}\">{}</td></tr>", lineNumber,
                     lineNumber, lineNumber, fmt::XMLEscape{line});
        lineNumber++;
    }
    *outs << R"#(</tbody>
</table>
</div>
</body>
</html>
)#";
}

} // namespace web
} // namespace ply
