/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-test/TestSuite.h>

namespace ply {
namespace test {

struct Case {
    StringView name;
    void (*func)();
};

Array<Case>& get_cases() {
    static Array<Case> cases;
    return cases;
}

Register::Register(StringView name, void (*func)()) {
    get_cases().append({name, func});
}

struct TestState {
    bool success = true;
};

TestState g_test_state;

bool check(bool cond) {
    if (!cond) {
        g_test_state.success = false;
    }
    return cond;
}

bool run() {
    u32 num_passed = 0;
    const auto& test_cases = get_cases();

    for (u32 i = 0; i < test_cases.num_items(); i++) {
        Console.out().format("[{}/{}] {}... ", (i + 1), test_cases.num_items(),
                             test_cases[i].name);
        g_test_state.success = true;
#if PLY_USE_DLMALLOC
        auto begin_stats = Heap.get_stats();
#endif
        test_cases[i].func();
#if PLY_USE_DLMALLOC
        // Check for memory leaks
        auto end_stats = Heap.get_stats();
        if (begin_stats.in_use_bytes != end_stats.in_use_bytes) {
            g_test_state.success = false;
        }
#endif
        Console.out() << (g_test_state.success ? StringView{"success\n"}
                                               : "***FAIL***\n");
        if (g_test_state.success) {
            num_passed++;
        }
    }
    float frac = 1.f;
    if (test_cases.num_items() > 0) {
        frac = (float) num_passed / test_cases.num_items();
    }
    Console.out().format("{}/{} test cases passed ({}%)\n", num_passed,
                         test_cases.num_items(), frac * 100.f);

    return num_passed == test_cases.num_items();
}

} // namespace test
} // namespace ply
