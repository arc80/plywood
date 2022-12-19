/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include "core.h"

char* GlyphData[] = {
// clang-format off
    "A ,,,, B,,,,, C ,,,, D,,,,, E,,,,,F,,,,,G ,,,, H,,  ,,I,,,,J    ,,K,,  ,,L,,   M,,   ,,",
    " ##  ## ##  ## ##  ** ##  ## ##    ##    ##  ** ##  ##  ##      ## ##,#*  ##    ###,###",
    " ##**## ##**#, ##     ##  ## ##**  ##**  ## *## ##**##  ##  ,,  ## ###,   ##    ##*#*##",
    " ##  ## ##,,#* *#,,#* ##,,#* ##,,, ##    *#,,## ##  ## ,##, *#,,#* ## *#, ##,,, ##   ##",
    "                                                                                       ",
    "N,,  ,,O ,,,, P,,,,, Q ,,,, R,,,,, S ,,,, T,,,,,,U,,  ,,V,,   ,,W,,   ,,X,,  ,,Y,,  ,,Z,,,,,,",
    " ### ## ##  ## ##  ## ##  ## ##  ## ##  **   ##   ##  ## ##   ## ## , ## *#,,#* ##  ##    ,#*",
    " ##*### ##  ## ##***  ##  ## ##**#,  ***#,   ##   ##  ##  ## ##  *#,#,#*  ,##,   *##*   ,#*  ",
    " ##  ## *#,,#* ##     *#,,#* ##  ## *#,,#*   ##   *#,,#*   *#*    ##*##  ##  ##   ##   ##,,,,",
    "                          **                                                                 ",
    "a      b,,    c     d    ,,e      f  ,,,g      h,,    i,,j   ,,k,,    l,,, m        ",
    "  ,,,,  ##,,,   ,,,,  ,,,##  ,,,,   ##    ,,,,, ##,,,  ,,    ,, ##  ,,  ##  ,,,,,,, ",
    "  ,,,## ##  ## ##    ##  ## ##,,## *##*  ##  ## ##  ## ##    ## ##,#*   ##  ## ## ##",
    " *#,,## ##,,#* *#,,, *#,,## *#,,,   ##   *#,,## ##  ## ##    ## ## *#, ,##, ## ## ##",
    "                                          ,,,#*           *#,#*                        ",
    "n      o      p      q      r      s      t ,,  u      v       w        x      y      z      ",
    " ,,,,,   ,,,,  ,,,,,   ,,,,, ,,,,,   ,,,,  ,##,, ,,  ,, ,,   ,, ,,    ,, ,,  ,, ,,  ,, ,,,,,,",
    " ##  ## ##  ## ##  ## ##  ## ##  ** *#,,,   ##   ##  ## *#, ,#* ## ## ##  *##*  ##  ##   ,#* ",
    " ##  ## *#,,#* ##,,#* *#,,## ##      ,,,#*  *#,, *#,,##   *#*    ##**##  ,#**#, *#,,## ,##,,,",
    "               ##         ##                                                     ,,,#*       ",
    "0 ,,,, 1 ,, 2 ,,,, 3 ,,,, 4   ,,, 5,,,,,,6 ,,,, 7,,,,,,8 ,,,, 9 ,,,, ",
    " ## ,## *##  **  ## **  ##  ,#*##  ##     ##         ## ##  ## ##  ##",
    " ##* ##  ##   ,#**    **#, ##,,##, ****#, ##**#,   ,#*  ,#**#,  ***##",
    " *#,,#* ,##, ##,,,, *#,,#*     ##  *#,,#* *#,,#*   ##   *#,,#*  ,,,#*",
    "                                                                     ",
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
    u32 numRows = PLY_STATIC_ARRAY_SIZE(GlyphData) / BigGlyph::height;
    for (u32 i = 0; i < numRows; i++) {
        char* row = GlyphData[i * BigGlyph::height];
        char startCol = 0;
        for (u32 j = startCol + 1;; j++) {
            if (StringView{" ,#*"}.findByte(row[j]) < 0) {
                char c = row[startCol];
                glyphs[c].row = i;
                glyphs[c].col = startCol + 1;
                glyphs[c].width = j - startCol - 1;
                startCol = j;
            }
            if (row[j] == 0)
                break;
        }
    }

    OutStream outs = StdOut::text();
    for (u32 i = 0; i < BigGlyph::height; i++) {
        outs << "// ";
        for (u32 j = 0; j < text.numBytes; j++) {
            // Look up glyph
            char c = text[j];
            if (c >= glyphs.numItems())
                continue;
            const BigGlyph& glyph = glyphs[c];
            if (glyph.width == 0)
                continue;

            // Print current row of glyph
            outs << ' ';
            const char* data = GlyphData[glyph.row * BigGlyph::height + i] + glyph.col;
            for (u32 k = 0; k < glyph.width; k++) {
                char p = data[k];
                if (p == ' ') {
                    outs << ' ';
                } else if (p == ',') {
                    outs << u8"▄";
                } else if (p == '#') {
                    outs << u8"█";
                } else if (p == '*') {
                    outs << u8"▀";
                }
            }
        }
        outs << '\n';
    }
}
