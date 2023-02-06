/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Core.h>
#include <web-common/SourceCode.h>

namespace ply {
namespace web {

PLY_NO_INLINE void SourceCode::serve(const SourceCode* params, StringView request_path,
                                     ResponseIface* response_iface) {
    // FIXME: Use FileSystem_Virtual and make really, really sure an adversary can't
    // read files outside the root_dir
    String norm_request_path = Path.normalize(request_path.ltrim(Path.is_sep_byte));
    Owned<InStream> ins = FileSystem::native()
                              ->open_text_for_read_autodetect(
                                  Path.join(params->root_dir, norm_request_path))
                              .first;
    if (!ins) {
        // file could not be loaded
        response_iface->respond_generic(ResponseCode::NotFound);
        return;
    }

    OutStream* outs = response_iface->begin_response_header(ResponseCode::OK);
    *outs << "Content-Type: text/html\r\n\r\n";
    response_iface->end_response_header();
    outs->format(R"#(<!DOCTYPE html>
<html>
<head>
<title>{}</title>
)#",
                 fmt::XMLEscape{Path.split(norm_request_path).second});
    *outs
        << R"#(<link href="/static/stylesheet.css" rel="stylesheet" type="text/css" />
<script>
var highlighted = null;
function highlight(element_id) {
    if (highlighted) {
        highlighted.style.background = "";
    }
    highlighted = document.get_element_by_id("LC" + element_id);
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
    u32 line_number = 1;
    while (String line = ins->read_string<fmt::Line>()) {
        outs->format("<tr><td id=\"L{}\">{}</td><td id=\"LC{}\">{}</td></tr>",
                     line_number, line_number, line_number, fmt::XMLEscape{line});
        line_number++;
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
