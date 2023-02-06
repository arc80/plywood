/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include "core.h"

//  ▄▄     ▄▄          ▄▄▄                ▄▄
//  ██▄▄▄  ▄▄  ▄▄▄▄▄  ██    ▄▄▄▄  ▄▄▄▄▄  ▄██▄▄
//  ██  ██ ██ ██  ██ ▀██▀▀ ██  ██ ██  ██  ██
//  ██▄▄█▀ ██ ▀█▄▄██  ██   ▀█▄▄█▀ ██  ██  ▀█▄▄
//             ▄▄▄█▀

char* GlyphData[] = {
    // clang-format off
    "A ,,,, B,,,,, C ,,,, D,,,,, E,,,,,F,,,,,G ,,,, H,,  ,,I,,,,J    ,,K,,  ,,L,,   M,,   ,,",
    " ##  ## ##  ## ##  `` ##  ## ##    ##    ##  `` ##  ##  ##      ## ##,#`  ##    ###,###",
    " ##``## ##``#, ##     ##  ## ##``  ##``  ## `## ##``##  ##  ,,  ## ###,   ##    ##`#`##",
    " ##  ## ##,,#` `#,,#` ##,,#` ##,,, ##    `#,,## ##  ## ,##, `#,,#` ## `#, ##,,, ##   ##",
    "                                                                                       ",
    "N,,  ,,O ,,,, P,,,,, Q ,,,, R,,,,, S ,,,, T,,,,,,U,,  ,,V,,   ,,W,,    ,,X,,  ,,Y,,  ,,Z,,,,,,",
    " ### ## ##  ## ##  ## ##  ## ##  ## ##  ``   ##   ##  ## ##   ## ## ,, ## `#,,#` ##  ##    ,#`",
    " ##`### ##  ## ##```  ##  ## ##``#,  ```#,   ##   ##  ##  ## ##  `#,##,#`  ,##,   `##`   ,#`  ",
    " ##  ## `#,,#` ##     `#,,#` ##  ## `#,,#`   ##   `#,,#`   `#`    ##``##  ##  ##   ##   ##,,,,",
    "                          ``                                                                  ",
    "a      b,,    c     d    ,,e      f  ,,,g      h,,    i,,j   ,,k,,    l,,, m        ",
    "  ,,,,  ##,,,   ,,,,  ,,,##  ,,,,   ##    ,,,,, ##,,,  ,,    ,, ##  ,,  ##  ,,,,,,, ",
    "  ,,,## ##  ## ##    ##  ## ##,,## `##`` ##  ## ##  ## ##    ## ##,#`   ##  ## ## ##",
    " `#,,## ##,,#` `#,,, `#,,## `#,,,   ##   `#,,## ##  ## ##    ## ## `#, ,##, ## ## ##",
    "                                          ,,,#`           `#,#`                        ",
    "n      o      p      q      r      s      t ,,  u      v       w        x      y      z      ",
    " ,,,,,   ,,,,  ,,,,,   ,,,,, ,,,,,   ,,,,  ,##,, ,,  ,, ,,   ,, ,,    ,, ,,  ,, ,,  ,, ,,,,,,",
    " ##  ## ##  ## ##  ## ##  ## ##  `` `#,,,   ##   ##  ## `#, ,#` ## ## ##  `##`  ##  ##   ,#` ",
    " ##  ## `#,,#` ##,,#` `#,,## ##      ,,,#`  `#,, `#,,##   `#`    ##``##  ,#``#, `#,,## ,##,,,",
    "               ##         ##                                                     ,,,#`       ",
    "0 ,,,, 1 ,, 2 ,,,, 3 ,,,, 4   ,,, 5,,,,,,6 ,,,, 7,,,,,,8 ,,,, 9 ,,,, _     ",
    " ## ,## `##  ``  ## ``  ##  ,#`##  ##     ##         ## ##  ## ##  ##      ",
    " ##` ##  ##   ,#``    ``#, ##,,##, ````#, ##``#,   ,#`  ,#``#,  ```##      ",
    " `#,,#` ,##, ##,,,, `#,,#`     ##  `#,,#` `#,,#`   ##   `#,,#`  ,,,#` ,,,,,",
    "                                                                           ",
    // clang-format on
};

struct BigGlyph {
    u32 row = 0;
    u32 col = 0;
    u32 width = 0;
    static constexpr u32 height = 5;
};

void print_bigfont(StringView text) {
    Array<BigGlyph> glyphs;
    glyphs.resize(128);
    u32 num_rows = PLY_STATIC_ARRAY_SIZE(GlyphData) / BigGlyph::height;
    for (u32 i = 0; i < num_rows; i++) {
        char* row = GlyphData[i * BigGlyph::height];
        char start_col = 0;
        for (u32 j = start_col + 1;; j++) {
            if (StringView{" ,#`"}.find_byte(row[j]) < 0) {
                char c = row[start_col];
                glyphs[c].row = i;
                glyphs[c].col = start_col + 1;
                glyphs[c].width = j - start_col - 1;
                start_col = j;
            }
            if (row[j] == 0)
                break;
        }
    }

    OutStream out = Console.out(CM_Text);
    for (u32 i = 0; i < BigGlyph::height; i++) {
        out << "// ";
        for (u32 j = 0; j < text.num_bytes; j++) {
            // Look up glyph
            char c = text[j];
            if (c >= glyphs.num_items())
                continue;
            const BigGlyph& glyph = glyphs[c];
            if (glyph.width == 0)
                continue;

            // Print current row of glyph
            out << ' ';
            const char* data = GlyphData[glyph.row * BigGlyph::height + i] + glyph.col;
            for (u32 k = 0; k < glyph.width; k++) {
                char p = data[k];
                if (p == ' ') {
                    out << ' ';
                } else if (p == ',') {
                    out << u8"▄";
                } else if (p == '#') {
                    out << u8"█";
                } else if (p == '`') {
                    out << u8"▀";
                }
            }
        }
        out << '\n';
    }
}

//                         ▄▄▄  ▄▄▄  ▄▄
//   ▄▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄   ██   ██  ██▄▄▄   ▄▄▄▄  ▄▄  ▄▄
//  ▀█▄▄▄  ██ ██ ██  ▄▄▄██  ██   ██  ██  ██ ██  ██  ▀██▀
//   ▄▄▄█▀ ██ ██ ██ ▀█▄▄██ ▄██▄ ▄██▄ ██▄▄█▀ ▀█▄▄█▀ ▄█▀▀█▄
//

void print_smallbox(StringView text) {
    u32 num_codepoints = text.num_codepoints(UTF8);
    String bar = StringView{u8"━"} * num_codepoints;

    OutStream out = Console.out();
    out.format(u8"// ┏━━{}━━┓\n", bar);
    out.format(u8"// ┃  {}  ┃\n", text);
    out.format(u8"// ┗━━{}━━┛\n", bar);
}
