/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Base.h>

using namespace ply;

template <typename T>
void test_parse(OutStream* outs, StringView type, StringView str, u32 radix) {
    ViewInStream ins{str};
    T result = ins.parse<T>(fmt::Radix{radix});
    String back_to_string = String::from(fmt::WithRadix{result, radix});
    outs->format("Parsing \"{}\" as {}, radix {} -> \"{}\", {}\n", str, type, radix,
                 back_to_string, ins.any_parse_error() ? "failed" : "success");
}

StringView str_list[] = {
    "123",
    "24000000",
    "7fffffff",
    "80000000",
    "ffffffff",
    "100000000",
    "100000001",
    "214748364",
    "214748365",
    "240000001",
    "429496729",
    "429496730",
    "2400000000000000",
    "7fffffffffffffff",
    "8000000000000000",
    "ffffffffffffffff",
    "10000000000000000",
    "10000000000000001",
    "24000000000000001",
    "922337203685477580",
    "922337203685477581",
    "1844674407370955161",
    "1844674407370955162",
};

int main() {
    MemOutStream mout;
    for (u32 i = 0; i < PLY_STATIC_ARRAY_SIZE(str_list); i++) {
        for (u32 s = 0; s < 2; s++) {
            String str = str_list[i];
            if (s == 1) {
                str = StringView{"-"} + str;
            }
            for (u32 radix : {10, 16}) {
                test_parse<u32>(&mout, "u32", str, radix);
                test_parse<s32>(&mout, "s32", str, radix);
                test_parse<u64>(&mout, "u64", str, radix);
                test_parse<s64>(&mout, "s64", str, radix);
            }
        }
    }
    FileSystem.make_dirs_and_save_text_if_different(
        Path.join(Workspace.path, "repos/plywood/src/apps/StringReaderTest/result.txt"),
        mout.move_to_string(), TextFormat::platform_preference());
    return 0;
}
