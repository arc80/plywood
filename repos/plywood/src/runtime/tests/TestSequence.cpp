/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/container/Sequence.h>
#include <ply-test/TestSuite.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX Sequence_

struct Fibonacci {
    u32 a = 0;
    u32 b = 1;
    PLY_INLINE u32 next() {
        u32 r = b;
        b += a;
        a = r;
        return r;
    }
};

void Sequence_testBeginWriteView(u32 num) {
    Sequence<u32> list;
    u32 lastFib = 0;
    {
        Fibonacci fib;
        while (num > 0) {
            ArrayView<u32> view = list.beginWriteViewNoConstruct();
            u32 itemsToWrite = min(num, view.numItems);
            for (u32 i = 0; i < itemsToWrite; i++) {
                view[i] = fib.next();
            }
            list.endWrite(itemsToWrite);
            num -= itemsToWrite;
        }
        lastFib = fib.next();
    }

    {
        Fibonacci fib;
        for (u32 v : list) {
            PLY_TEST_CHECK(v == fib.next());
        }
        PLY_TEST_CHECK(lastFib == fib.next());
    }
}

PLY_TEST_CASE("Sequence beginWriteView small loop") {
    Sequence_testBeginWriteView(10);
}

PLY_TEST_CASE("Sequence beginWriteView big loop") {
    Sequence_testBeginWriteView(10000);
}

void Sequence_testAppend(u32 num) {
    Sequence<u32> list;
    u32 lastFib = 0;
    {
        Fibonacci fib;
        for (u32 i = 0; i < num; i++) {
            list.append(fib.next());
        }
        lastFib = fib.next();
    }

    {
        Fibonacci fib;
        for (u32 v : list) {
            PLY_TEST_CHECK(v == fib.next());
        }
        PLY_TEST_CHECK(lastFib == fib.next());
    }
}

PLY_TEST_CASE("Sequence append small loop") {
    Sequence_testAppend(10);
}

PLY_TEST_CASE("Sequence append big loop") {
    Sequence_testAppend(10000);
}

void Sequence_testToArray(u32 num) {
    Sequence<u32> list;
    u32 lastFib = 0;
    {
        Fibonacci fib;
        for (u32 i = 0; i < num; i++) {
            list.append(fib.next());
        }
        lastFib = fib.next();
    }

    {
        Array<u32> arr = list.moveToArray();
        Fibonacci fib;
        for (u32 v : arr) {
            PLY_TEST_CHECK(v == fib.next());
        }
        PLY_TEST_CHECK(lastFib == fib.next());
    }
}

PLY_TEST_CASE("Sequence to small array") {
    Sequence_testToArray(10);
}

PLY_TEST_CASE("Sequence to big array") {
    Sequence_testToArray(10000);
}

void TestBuffer_testDestructors(u32 num, bool move) {
    struct Dtor {
        u32* counter = nullptr;
        ~Dtor() {
            *this->counter += 1;
        }
    };

    u32 counter = 0;
    {
        Array<Dtor> arr;
        {
            Sequence<Dtor> list;
            for (u32 i = 0; i < num; i++) {
                list.append(&counter);
            }
            if (move) {
                arr = list.moveToArray();
            }
        }
        PLY_TEST_CHECK(counter == (move ? 0 : num));
    }
    PLY_TEST_CHECK(counter == num);
}

PLY_TEST_CASE("Sequence small destructors") {
    TestBuffer_testDestructors(10, false);
}

PLY_TEST_CASE("Sequence big destructors") {
    TestBuffer_testDestructors(10000, false);
}

PLY_TEST_CASE("Sequence move small destructors") {
    TestBuffer_testDestructors(10, true);
}

PLY_TEST_CASE("Sequence move big destructors") {
    TestBuffer_testDestructors(10000, true);
}

} // namespace tests
} // namespace ply
