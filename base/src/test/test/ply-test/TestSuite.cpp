/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-test/TestSuite.h>

namespace ply {
namespace test {

struct Case {
    StringView name;
    void (*func)();
};

Array<Case>& getCases() {
    static Array<Case> cases;
    return cases;
}

Register::Register(StringView name, void (*func)()) {
    getCases().append({name, func});
}

struct TestState {
    bool success = true;
};

TestState gTestState;

bool check(bool cond) {
    if (!cond) {
        gTestState.success = false;
    }
    return cond;
}

bool run() {
    u32 numPassed = 0;
    const auto& testCases = getCases();

    for (u32 i = 0; i < testCases.numItems(); i++) {
        Console.out().format("[{}/{}] {}... ", (i + 1), testCases.numItems(), testCases[i].name);
        gTestState.success = true;
#if PLY_USE_DLMALLOC
        auto beginStats = Heap.getStats();
#endif
        testCases[i].func();
#if PLY_USE_DLMALLOC
        // Check for memory leaks
        auto endStats = Heap.getStats();
        if (beginStats.inUseBytes != endStats.inUseBytes) {
            gTestState.success = false;
        }
#endif
        Console.out() << (gTestState.success ? StringView{"success\n"} : "***FAIL***\n");
        if (gTestState.success) {
            numPassed++;
        }
    }
    float frac = 1.f;
    if (testCases.numItems() > 0) {
        frac = (float) numPassed / testCases.numItems();
    }
    Console.out().format("{}/{} test cases passed ({}%)\n", numPassed, testCases.numItems(),
                          frac * 100.f);

    return numPassed == testCases.numItems();
}

} // namespace test
} // namespace ply
