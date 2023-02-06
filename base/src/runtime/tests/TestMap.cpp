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

#define PLY_TEST_CASE_PREFIX Map_

//--------------------------------
// Constructors
//--------------------------------
PLY_TEST_CASE("Map tests") {
    for (u32 num_items : {2, 3, 4, 5, 6, 8, 10, 20, 50, 200, 300, 1000}) {
        u32 seed = avalanche(num_items);

        // Populate a map
        Map<Label, u32> map;
        for (u32 i = 0; i < num_items; i++) {
            u32 key = avalanche(i + seed);
            bool was_found = false;
            *map.insert_or_find(Label{key}, &was_found) = i * seed;
            PLY_TEST_CHECK(!was_found);
            map.insert_or_find(Label{key}, &was_found);
            PLY_TEST_CHECK(was_found);
        }
        PLY_TEST_CHECK(map.items.num_items() == num_items);

        // Find each item in map
        for (u32 i = 0; i < num_items; i++) {
            u32 key = avalanche(i + seed);
            u32* value = map.find(Label{key});
            if (PLY_TEST_CHECK(value)) {
                PLY_TEST_CHECK(*value == i * seed);
            }
        }

        // Iterate over map
        Array<bool> found;
        found.resize(num_items);
        for (bool& f : found) {
            f = false;
        }
        for (auto item : map) {
            u32 i = deavalanche(item.key.idx) - seed;
            if (PLY_TEST_CHECK(i < num_items)) {
                PLY_TEST_CHECK(item.value == i * seed);
                PLY_TEST_CHECK(!found[i]);
                found[i] = true;
            }
        }
    }
}

} // namespace tests
} // namespace ply
