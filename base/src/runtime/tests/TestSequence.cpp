/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>
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
    Sequence<u32> seq;
    u32 last_fib = 0;
    {
        Fibonacci fib;
        while (num > 0) {
            ArrayView<u32> view = seq.begin_write_view_no_construct();
            u32 items_to_write = min(num, view.num_items);
            for (u32 i = 0; i < items_to_write; i++) {
                view[i] = fib.next();
            }
            seq.end_write(items_to_write);
            num -= items_to_write;
        }
        last_fib = fib.next();
    }

    {
        Fibonacci fib;
        for (u32 v : seq) {
            PLY_TEST_CHECK(v == fib.next());
        }
        PLY_TEST_CHECK(last_fib == fib.next());
    }
}

PLY_TEST_CASE("Sequence beginWriteView small loop") {
    Sequence_testBeginWriteView(10);
}

PLY_TEST_CASE("Sequence beginWriteView big loop") {
    Sequence_testBeginWriteView(10000);
}

void Sequence_testAppend(u32 num) {
    Sequence<u32> seq;
    u32 last_fib = 0;
    {
        Fibonacci fib;
        for (u32 i = 0; i < num; i++) {
            seq.append(fib.next());
        }
        last_fib = fib.next();
    }

    {
        Fibonacci fib;
        for (u32 v : seq) {
            PLY_TEST_CHECK(v == fib.next());
        }
        PLY_TEST_CHECK(last_fib == fib.next());
    }
}

PLY_TEST_CASE("Sequence append small loop") {
    Sequence_testAppend(10);
}

PLY_TEST_CASE("Sequence append big loop") {
    Sequence_testAppend(10000);
}

void Sequence_testToArray(u32 num) {
    Sequence<u32> seq;
    u32 last_fib = 0;
    {
        Fibonacci fib;
        for (u32 i = 0; i < num; i++) {
            seq.append(fib.next());
        }
        last_fib = fib.next();
    }

    {
        Array<u32> arr = seq.move_to_array();
        Fibonacci fib;
        for (u32 v : arr) {
            PLY_TEST_CHECK(v == fib.next());
        }
        PLY_TEST_CHECK(last_fib == fib.next());
    }
}

PLY_TEST_CASE("Sequence to small array") {
    Sequence_testToArray(10);
}

PLY_TEST_CASE("Sequence to big array") {
    Sequence_testToArray(10000);
}

void Sequence_testDestructors(u32 num, bool move) {
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
            Sequence<Dtor> seq;
            for (u32 i = 0; i < num; i++) {
                seq.append(&counter);
            }
            if (move) {
                arr = seq.move_to_array();
            }
        }
        PLY_TEST_CHECK(counter == (move ? 0 : num));
    }
    PLY_TEST_CHECK(counter == num);
}

PLY_TEST_CASE("Sequence small destructors") {
    Sequence_testDestructors(10, false);
}

PLY_TEST_CASE("Sequence big destructors") {
    Sequence_testDestructors(10000, false);
}

PLY_TEST_CASE("Sequence move small destructors") {
    Sequence_testDestructors(10, true);
}

PLY_TEST_CASE("Sequence move big destructors") {
    Sequence_testDestructors(10000, true);
}

void Sequence_testPopTail(u32 num_items, u32 num_to_pop) {
    struct Dtor {
        u32* counter = nullptr;
        ~Dtor() {
            *this->counter += 1;
        }
    };

    u32 counter = 0;
    {
        Sequence<Dtor> seq;
        for (u32 i = 0; i < num_items; i++) {
            seq.append(&counter);
        }
        seq.pop_tail(num_to_pop);
        PLY_TEST_CHECK(counter == num_to_pop);
    }
    PLY_TEST_CHECK(counter == num_items);
}

PLY_TEST_CASE("Sequence small popTail") {
    Sequence_testPopTail(10, 1);
}

PLY_TEST_CASE("Sequence big popTail") {
    Sequence_testPopTail(10000, 5000);
}

} // namespace tests
} // namespace ply
