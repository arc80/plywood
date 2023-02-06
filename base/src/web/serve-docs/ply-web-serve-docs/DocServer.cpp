/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-serve-docs/Core.h>
#include <ply-web-serve-docs/DocServer.h>
#include <pylon/Parse.h>
#include <pylon-reflect/Import.h>

namespace ply {
namespace web {

void dump_contents(OutStream* outs, const Contents* node,
                   ArrayView<const Contents*> expand_to) {
    bool is_expanded = false;
    bool is_selected = false;
    if (expand_to && expand_to.back() == node) {
        is_expanded = true;
        expand_to = expand_to.shortened_by(1);
        is_selected = expand_to.is_empty();
    } else {
        expand_to = {};
    }

    if (node->link_destination) {
        outs->format("<a href=\"{}\">", node->link_destination);
    }
    *outs << "<li";
    bool any_classes = false;
    auto add_class = [&](StringView name) {
        *outs << (any_classes ? StringView{" "} : StringView{" class=\""}) << name;
        any_classes = true;
    };
    add_class("selectable");
    if (node->children) {
        add_class("caret");
        if (is_expanded) {
            add_class("caret-down");
        }
    }
    if (is_selected) {
        add_class("selected");
    }
    *outs << (any_classes ? StringView{"\">"} : StringView{">"});
    *outs << "<span>" << fmt::XMLEscape{node->title} << "</span>";
    *outs << "</li>\n";
    if (node->link_destination) {
        *outs << "</a>";
    }
    if (node->children) {
        outs->format("<ul class=\"nested{}\">\n",
                     is_expanded ? StringView{" active"} : StringView{});
        for (const Contents* child : node->children) {
            dump_contents(outs, child, expand_to);
        }
        *outs << "</ul>\n";
    }
}

void DocServer::init(StringView data_root) {
    FileSystem* fs = FileSystem::native();

    this->data_root = data_root;
    this->contents_path = Path.join(data_root, "contents.pylon");
    FileStatus contents_status = fs->get_file_status(this->contents_path);
    if (contents_status.result == FSResult::OK) {
        this->reload_contents();
        this->contents_mod_time = contents_status.modification_time;
    }
}

void populate_contents_map(HashMap<DocServer::ContentsTraits>& path_to_contents,
                           Contents* node) {
    if (node->link_destination) {
        path_to_contents.insert_or_find(node->link_destination)->node = node;
    }
    for (Contents* child : node->children) {
        child->parent = node;
        populate_contents_map(path_to_contents, child);
    }
}

void DocServer::reload_contents() {
    FileSystem* fs = FileSystem::native();

    String contents_pylon = fs->load_text(this->contents_path, TextFormat::unix_utf8());
    if (fs->last_result() != FSResult::OK) {
        // FIXME: Log an error here
        return;
    }

    Owned<pylon::Node> a_root = pylon::Parser{}.parse(contents_pylon).root;
    if (!a_root->is_valid()) {
        // FIXME: Log an error here
        return;
    }

    pylon::import_into(AnyObject::bind(&this->contents), a_root);

    this->path_to_contents = HashMap<ContentsTraits>{};
    for (Contents* node : this->contents) {
        populate_contents_map(this->path_to_contents, node);
    }
}

String get_page_source(DocServer* ds, StringView request_path,
                       ResponseIface* response_iface) {
    FileSystem* fs = FileSystem::native();
    if (Path.is_absolute(request_path)) {
        response_iface->respond_generic(ResponseCode::NotFound);
        return {};
    }
    String abs_path = Path.join(ds->data_root, "pages", request_path);
    ExistsResult exists = FileSystem.exists(abs_path);
    if (exists == ExistsResult::Directory) {
        abs_path = Path.join(abs_path, "index.html");
    } else {
        abs_path += ".html";
    }
    String page_html = fs->load_text(Path.join(ds->data_root, "pages", abs_path),
                                     TextFormat::unix_utf8());
    if (!page_html) {
        response_iface->respond_generic(ResponseCode::NotFound);
        return {};
    }
    return page_html;
}

void DocServer::serve(StringView request_path, ResponseIface* response_iface) {
    FileSystem* fs = FileSystem::native();

    // Check if contents.pylon has been updated:
    FileStatus contents_status = fs->get_file_status(this->contents_path);
    if (contents_status.result == FSResult::OK) {
        if (contents_status.modification_time !=
            this->contents_mod_time.load(MemoryOrder::Acquire)) {
            ply::LockGuard<ply::Mutex> guard{this->contents_mutex};
            if (contents_status.modification_time !=
                this->contents_mod_time.load(MemoryOrder::Relaxed)) {
                this->reload_contents();
            }
            this->contents_mod_time = contents_status.modification_time;
        }
    }

    if (!this->contents) {
        response_iface->respond_generic(ResponseCode::InternalError);
        return;
    }

    // Load page
    String page_html = get_page_source(this, request_path, response_iface);
    if (!page_html)
        return;
    ViewInStream vins{page_html};
    String page_title = vins.read_view<fmt::Line>().trim(is_white);

    // Figure out which TOC entries to expand
    Array<const Contents*> expand_to;
    {
        auto cursor = this->path_to_contents.find(
            request_path ? (StringView{"/docs/"} + request_path).view()
                         : StringView{"/"});
        if (cursor.was_found()) {
            const Contents* node = cursor->node;
            while (node) {
                expand_to.append(node);
                node = node->parent;
            }
        }
    }

    OutStream* outs = response_iface->begin_response_header(ResponseCode::OK);
    *outs << "Content-Type: text/html; charset=utf-8\r\n\r\n";
    response_iface->end_response_header();
    outs->format(R"#(<!DOCTYPE html>
<html>
<head>
<title>{}</title>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
)#",
                 page_title);
    *outs
        << R"#(<link href="/static/stylesheet.css?1" rel="stylesheet" type="text/css" />
<link rel="icon" href="/static/favicon@32x32.png" sizes="32x32" />
<script src="/static/docs.js"></script>
</head>
<body>
  <div class="siteTitle">
    <a href="/"><img src="/static/logo.svg" id="logo"/></a>
    <span class="right"><span id="get-involved" class="button"><span class="text">Get Involved <span class="downcaret"></span></span></span><span id="three-lines" class="button"><span></span></span></span>
  </div>
  <div class="get-involved-popup">
    <div class="scroller">
      <div class="inner">
        <ul>
            <a href="https://discord.gg/WnQhuVF"><li><img src="/static/discord-button.svg" /> <span>Join the Discord Server</span></li></a>
            <a href="https://github.com/arc80/plywood"><li><img src="/static/github-button.svg" /> <span>View on GitHub</span></li></a>
        </ul>
      </div>
    </div>
  </div>
  <div class="sidebar">
    <div class="scroller">
      <div class="inner">
        <ul>
)#";
    for (const Contents* node : this->contents) {
        dump_contents(outs, node, expand_to);
    }
    outs->format(R"(
        </ul>
      </div>
    </div>
  </div>
  <article class="content" id="article">
<h1>{}</h1>
)",
                 page_title);
    *outs << vins.view_available();
    *outs << R"(
  </article>
</body>
</html>
)";
}

void DocServer::serve_content_only(StringView request_path,
                                   ResponseIface* response_iface) {
    String page_html = get_page_source(this, request_path, response_iface);
    if (!page_html)
        return;
    OutStream* outs = response_iface->begin_response_header(ResponseCode::OK);
    ViewInStream vins{page_html};
    String page_title = vins.read_view<fmt::Line>().trim(is_white);
    *outs << "Content-Type: text/html\r\n\r\n";
    response_iface->end_response_header();
    outs->format("{}\n<h1>{}</h1>\n", page_title, page_title);
    *outs << vins.view_available();
}

} // namespace web
} // namespace ply
